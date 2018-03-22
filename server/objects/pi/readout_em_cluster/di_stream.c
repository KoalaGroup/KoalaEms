/*
 * readout_em_cluster/di_stream.c
 * created 2011-04-21 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: di_stream.c,v 1.3 2017/10/27 21:10:48 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sconf.h>
#include <debug.h>
#include <unsoltypes.h>
#include "xdrstring.h"
#include <rcs_ids.h>
#include "di_stream.h"
#include "../../../main/scheduler.h"
#include "datains.h"
#include "../../domain/dom_datain.h"
#include "../../../dataout/cluster/do_cluster.h"
#include "../readout.h"
#include "../../../commu/commu.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

#if 0
#define DEBUG
#endif

#define ID_PREFIX "stream://"

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

#define set_max(a, b) ((a)=(a)<(b)?(b):(a))

struct di_stream_data {
    /* socket information */
    int server; /* dataout is the passive part */
    //int connected;
    int s_path; /* server socket */
    int n_path; /* socket for data */
    struct seltaskdescr* accepttask; /* task for accept (if server) */
    int running; /* if not running: discard all received data */
    ems_u32 *idblob;
    int idlen;
    int maxdatalen; /* maximum payload size in BYTES */
    u_int8_t *data;

#if 0
    /* data information */
    u_int8_t  *data;
    u_int8_t  head[4]; /* received header (big endian!) */
    u_int32_t headnum; /* received bytes of header */
    u_int32_t bytes;   /* length of the opaque data; valid if headnum==4 */
    u_int32_t size;    /* bytes to be read;          valid if headnum==4 */
    u_int32_t space;   /* allocated size of data */
    u_int32_t num;     /* received bytes of data */
#endif
};

RCS_REGISTER(cvsid, "objects/pi/readout_em_cluster")

extern ems_u32* outptr;

/*****************************************************************************/
static void
possible_readout_error(void)
{
    if (readout_active==Invoc_active) {
        complain("di_opaque: fatal error");
        fatal_readout_error();
    } else {
        complain("di_opaque: non-fatal error (readout not active)");
    }
}
/*****************************************************************************/
/*
 * struct cluster {
 *   int size;                        // Anzahl aller folgenden Worte
 *   const int endiantest=0x12345678;
 *   union switch (clustertypes type) {
 *     case clusterty_async_data2:
 *       struct {
 *         int options<>;
 *         int flags;                   // noch nicht benutzt
 *         int fragment_id;             // z.Z.==0; noch keine Fragmente
 *         opaque data<>;
 *       } async_data;
 *   } content;
 * };
 */
/*
 * write_to_cluster will return:
 *  -1: fatal error (i.e. size too large)
 *   0: no error, but temporarily no cluster available
 *   1: succes
 */
static int
write_to_cluster(int di_idx, int size, struct timeval *now)
{
    struct di_stream_data *private=
            (struct di_stream_data*)datain_cl[di_idx].private;
    struct Cluster *cl;
    size_t clustersize;
    int nidx, xplen;

    if (!private->running) {
        /* do nothin, just return */
        return 1;
    }

    /* calculate size of cluster */
      /* length of the payload */
    xplen=(size+3)/4;
      /*  the cluster header needs 10 words:
       *    size, endiantest, type
       *    optionsize, optionflags, timestamp (2 words)
       *    flags, fragment_id, version
       *  and we need space for the identifier and the payload
       */
    clustersize=10+private->idlen+1+xplen;

    if (clustersize>cluster_max) {
        complain("di_stream_read: data size (%d words) too large", xplen);
        return -1;
    }


    /* if there are no clusters available we just throw the messages away
       we do not stop reading the mqtt messages */
    cl=clusters_create(clustersize, di_idx, "di_stream data");
    if (!cl) {
        complain("di_stream_read: no space available, message lost");
        return -1;
    }

    /* complete the internal header */
    cl->size=clustersize;
    cl->type=clusterty_async_data2;
    cl->flags=0;
    cl->optsize=3;
    cl->ved_id=0;

    /* fill the external header */
    nidx=0;
    cl->data[nidx++]=htonl(cl->size-1);
    cl->data[nidx++]=htonl(0x12345678);
    cl->data[nidx++]=htonl(cl->type);
    cl->data[nidx++]=htonl(cl->optsize);
    cl->data[nidx++]=htonl(ClOPTFL_TIME);
    cl->data[nidx++]=htonl(now->tv_sec);
    cl->data[nidx++]=htonl(now->tv_usec);
    cl->data[nidx++]=htonl(cl->flags);
    cl->data[nidx++]=htonl(0); /* fragment_id */
    cl->data[nidx++]=htonl(0); /* version */

    /* copy the identifier */
    memcpy(cl->data+nidx, private->idblob, private->idlen*4);
    nidx+=private->idlen;

    /* copy the payload */
    cl->data[nidx+xplen]=0; /* the unused bytes at the end must be 0 */
    cl->data[nidx++]=htonl(size);
    memcpy(cl->data+nidx, private->data, size);

    cl->swapped=cl->data[1]==0x78563412;

    clusters_deposit(cl);

    return 0;
}
/*****************************************************************************/
/*
 * !!! Daten muessen unabhaengig von 'readout' gelesen und notfalls
 *     weggeworfen werden !!!
 *
 * We do not expect any structure in the data. We issue just one 'read' on
 * store the data in one cluster. The data consumer has to deal with all this
 * mess.
 */
static void
di_stream_read(int path, enum select_types selected, union callbackdata data)
{
    int di_idx=data.i;
    Datain_cl* di_cl=datain_cl+di_idx;
    struct di_stream_data* private=
        (struct di_stream_data*)datain_cl[di_idx].private;
    struct timeval now;
    int res;

    T(readout_em_opaque/di_opaque.c:di_stream_read)

#ifdef DEBUG
    printf("di_stream_read\n");
#endif

    gettimeofday(&now, 0);

    if (di_cl->suspended) {
        complain("di_stream_read called while suspended");
        di_cl->error=-1;
        di_cl->status=Invoc_error;
        sched_delete_select_task(di_cl->seltask);
        di_cl->seltask=0;
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_InDev,
                di_idx, 3, errno);
        possible_readout_error();
        return;
    }

    if (selected!=select_read) {
        complain("di_stream_read: selected=%d", selected);
        di_cl->error=-1;
        di_cl->status=Invoc_error;
        sched_delete_select_task(di_cl->seltask);
        di_cl->seltask=0;
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_InDev, di_idx, 2, selected);
        possible_readout_error();
        return;
    }

    /* read anything */
    res=read(path, private->data, private->maxdatalen);
    if (res<=0) {
        if (res<0) {
            if (errno==EINTR) /* no error */
                return;
            complain("di_stream_read: %s", strerror(errno));
        } else {
            complain("di_stream_read: broken socket");
        }
        di_cl->error=-1;
        di_cl->status=Invoc_error;
        sched_delete_select_task(di_cl->seltask);
        di_cl->seltask=0;
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_InDev,
                di_idx, 3, errno);
        possible_readout_error();
        return;
    }

    /*
     * we ignore all errors; the show must go on! 
     */
    (void)write_to_cluster(di_idx, res, &now);
}
/*****************************************************************************/
/*
 * This code (copied from di_cluster) is can be used for nonblocking connect.
 * But perhaps the blocking connect is good enough.
 */
#if 0
static void
di_sock_connect(int path, enum select_types selected, union callbackdata data)
{
int idx=data.i;
struct di_stream_data *private=datain_cl[idx].private;
T(readout_em_opaque/di_opaque.c:di_sock_connect)

sched_delete_select_task(datain_cl[idx].seltask);
datain_cl[idx].seltask=0;

if (selected&select_except)
  {
  ems_u32 buf[3];
  printf("Error in di_sock_connect.\n");
  datain_cl[idx].error=-1;
  datain_cl[idx].status=Invoc_error;
  buf[0]=rtErr_InDev;
  buf[1]=idx;
  buf[2]=8;
  send_unsolicited(Unsol_RuntimeError, buf, 3);
  possible_readout_error();
  }
else
  {
  socklen_t ilen;
  int val;

  ilen=sizeof(int);
  if (getsockopt(path, SOL_SOCKET, SO_ERROR, (char*)&val, &ilen)<0)
    {
    ems_u32 buf[4];
    datain_cl[idx].error=errno;
    printf("Error in di_sock_connect::getsockopt: %s\n", strerror(errno));
    datain_cl[idx].status=Invoc_error;
    buf[0]=rtErr_InDev;
    buf[1]=idx;
    buf[2]=9;
    buf[3]=errno;
    send_unsolicited(Unsol_RuntimeError, buf, 4);
    possible_readout_error();
    }
  else
    {
    if (val!=0)
      {
      ems_u32 buf[4];
      datain_cl[idx].error=val;
      printf("Bad status in di_sock_connect: %d\n", val);
      datain_cl[idx].status=Invoc_error;
      buf[0]=rtErr_InDev;
      buf[1]=idx;
      buf[2]=10;
      buf[3]=val;
      send_unsolicited(Unsol_RuntimeError, buf, 4);
      possible_readout_error();
      }
    else /* ok */
      {
      char name[100];
      printf("datain %d connected\n", idx);
      private->connected=1;
      snprintf(name, 100, "di_%02d_stream_read", idx);
      datain_cl[idx].seltask=sched_insert_select_task(di_stream_read, data,
            name, path, select_read|select_except, 1);
      if (datain_cl[idx].seltask==0)
        {
        ems_u32 buf[3];
        printf("di_sock_connect: can't create task.\n");
        datain_cl[idx].error=-1;
        datain_cl[idx].status=Invoc_error;
        buf[0]=rtErr_InDev;
        buf[1]=idx;
        buf[2]=11;
        send_unsolicited(Unsol_RuntimeError, buf, 3);
        possible_readout_error();
        }
      }
    }
  }
}
#endif
/*****************************************************************************/
/*
 * This code (copied from di_cluster) is can be used for a server socket.
 * But perhaps a client is enough.
 */
#if 0
static void
di_sock_accept(int path, enum select_types selected, union callbackdata data)
{
int idx=data.i;
struct di_stream_data *private=datain_cl[idx].private;
int newpath;
T(readout_em_opaque/di_opaque.c:di_sock_accept)

if (selected&select_except)
  {
  if (!private->connected)
    {
    ems_u32 buf[3];
    datain_cl[idx].error=-1;
    close(path);
    private->n_path=-1;
    sched_delete_select_task(private->accepttask);
    private->accepttask=0;
    datain_cl[idx].status=Invoc_error;
    buf[0]=rtErr_InDev;
    buf[1]=idx;
    buf[2]=12;
    send_unsolicited(Unsol_RuntimeError, buf, 3);
    possible_readout_error();
    }
  printf("Error in di_sock_accept.\n");
  }
else
  {
  struct sockaddr addr;
  socklen_t size=sizeof(struct sockaddr);
  newpath=accept(path, &addr, &size);
  if (newpath<0)
    {
    if (!private->connected)
      {
      ems_u32 buf[4];
      datain_cl[idx].error=errno;
      datain_cl[idx].status=Invoc_error;
      buf[0]=rtErr_InDev;
      buf[1]=idx;
      buf[2]=13;
      buf[3]=errno;
      send_unsolicited(Unsol_RuntimeError, buf, 4);
      possible_readout_error();
      printf("di_sock_accept(idx=%d): error: %s\n", idx, strerror(errno));
      }
    }
  else /* accept ok */
    {
    if (private->connected)
      {
      printf("di_sock_accept(di_idx=%d): already used.\n", idx);
      close(newpath);
      }
    else
      {
      printf("datain %d accepted\n", idx);
      if (fcntl(newpath, F_SETFD, FD_CLOEXEC)<0)
        {
        printf("di_opaque.c: fcntl(newpath, FD_CLOEXEC): %s\n", strerror(errno));
        }
      private->n_path=newpath;
      if (fcntl(private->n_path, F_SETFL, O_NDELAY)<0)
        {
        ems_u32 buf[4];
        datain_cl[idx].error=errno;
        printf("di_sock_accept: can't set to O_NDELAY: %s\n", strerror(errno));
        datain_cl[idx].status=Invoc_error;
        buf[0]=rtErr_InDev;
        buf[1]=idx;
        buf[2]=14;
        buf[3]=errno;
        send_unsolicited(Unsol_RuntimeError, buf, 4);
        possible_readout_error();
        }
      else /* fcntl ok */
        {
        if (datain_cl[idx].status==Invoc_active)
          {
          char name[100];
          snprintf(name, 100, "di_%02d_stream_read", idx);
          datain_cl[idx].seltask=sched_insert_select_task(di_stream_read, data,
              name, private->n_path, select_read|select_except, 1);
          if (datain_cl[idx].seltask==0)
            {
            ems_u32 buf[3];
            datain_cl[idx].error=-1;
            datain_cl[idx].status=Invoc_error;
            buf[0]=rtErr_InDev;
            buf[1]=idx;
            buf[2]=15;
            send_unsolicited(Unsol_RuntimeError, buf, 5);
            possible_readout_error();
            printf("di_sock_accept: can't create task.\n");
            }
          else /* task ok */
            {
            /* OK */
            private->connected=1;
            }
          }
        else /* !active */
          private->connected=1;
        }
      }
    }
  }
}
#endif
/*****************************************************************************/
static errcode
di_stream_sock_start(int idx)
{
    struct di_stream_data *private=datain_cl[idx].private;

#ifdef DEBUG
    printf("di_stream_sock_start\n");
#endif

    private->running=1;

    return OK;
}
/*****************************************************************************/
static errcode
di_stream_sock_stop(int idx)
{
    struct di_stream_data *private=datain_cl[idx].private;

#ifdef DEBUG
    printf("di_stream_sock_stop\n");
#endif

    private->running=0;

    return OK;
}
/*****************************************************************************/
static void
di_stream_sock_reactivate(int idx)
{
    T(readout_em_opaque/di_opaque.c:di_stream_sock_reactivate)

#ifdef DEBUG
    printf("di_stream_sock_reactivate\n");
#endif

    if (datain_cl[idx].seltask) {
        sched_select_task_reactivate(datain_cl[idx].seltask);
        datain_cl[idx].suspended=0;
    } else {
        printf("di_stream_sock_reactivate(idx=%d): no task\n", idx);
    }
}
/*****************************************************************************/
static void
di_stream_sock_cleanup(int idx)
{
    struct di_stream_data *private=
            (struct di_stream_data*)datain_cl[idx].private;
    T(readout_em_opaque/di_opaque.c:di_stream_sock_cleanup)

    printf("di_stream_sock_cleanup(%d)\n", idx);
    if (private->s_path>=0)
        close(private->s_path);
    if (private->n_path>=0)
        close(private->n_path);
    sched_delete_select_task(private->accepttask);
    sched_delete_select_task(datain_cl[idx].seltask);
    datain_cl[idx].seltask=0;
    free(private->idblob);
    free(private->data);
    free(datain_cl[idx].private);
    datain_cl[idx].private=0;
}
/*****************************************************************************/
static di_procs sock_procs={
    di_stream_sock_start,
    di_stream_sock_stop,
    di_stream_sock_start,
    di_stream_sock_stop,
    di_stream_sock_cleanup,
    di_stream_sock_reactivate,
    0 /*di_stream_sock_request*/
};
/*****************************************************************************/
static errcode
di_stream_sock_v4server(int idx)
{
printf("di_stream_sock_v4server: port=%d (not implemented yet)\n",
        datain[idx].addr.inetsock.port);
    return Err_AddrTypNotImpl;
}
/*****************************************************************************/
static errcode
di_stream_sock_v6server(int idx)
{
printf("di_stream_sock_v6server: service=%s (not implemented yet)\n",
        datain[idx].addr.inetV6sock.service);
    return Err_AddrTypNotImpl;
}
/*****************************************************************************/
static errcode
di_stream_sock_v4client(int idx)
{
    struct di_stream_data *private=
            (struct di_stream_data*)datain_cl[idx].private;
    struct sockaddr_in sa;
    int s;

printf("di_stream_sock_v4client: host=%08x port=%d\n",
        datain[idx].addr.inetsock.host, datain[idx].addr.inetsock.port);

    s=socket(AF_INET, SOCK_STREAM, 0);
    if (s<0) {
        *outptr++=errno;
        printf("di_stream_sock_v4client: can't open socket: %s\n",
                strerror(errno));
        return Err_System;
    }
    if (fcntl(s, F_SETFD, FD_CLOEXEC)<0) {
        printf("di_stream_sock_v4client: fcntl(FD_CLOEXEC): %s\n",
                strerror(errno));
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family=AF_INET;
    sa.sin_port=htons(datain[idx].addr.inetsock.port);
    sa.sin_addr.s_addr=htonl(datain[idx].addr.inetsock.host);
    if (connect(s, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))<0) {
        *outptr++=errno;
        printf("di_stream_sock_v4client: can't connect socket: %s\n",
                strerror(errno));
        close(s);
        return Err_System;
    }

    if (fcntl(s, F_SETFL, O_NDELAY)<0) {
        *outptr++=errno;
        printf("di_stream_sock_v4client: can't set to O_NDELAY: %s\n",
                strerror(errno));
        close(s);
        return Err_System;
    }
    private->n_path=s;

    return OK;
}
/*****************************************************************************/
static errcode
di_stream_sock_v6client(int idx)
{
    struct di_stream_data *private=
            (struct di_stream_data*)datain_cl[idx].private;
    struct addrinfo *a;
    int s=-1;

printf("di_stream_sock_v6client: node=%s service=%s\n",
        datain[idx].addr.inetV6sock.node, datain[idx].addr.inetV6sock.service);
    for (a=datain[idx].addr.inetV6sock.addr; a; a=a->ai_next) {
        s=socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (s<0) {
            complain("di_stream_sock_client: socket: %s", strerror(errno));
            continue;
        }
        if (connect(s, a->ai_addr, a->ai_addrlen)<0)
            complain("di_stream_sock_v6client: connect: %s", strerror(errno));
        else
            break;  /* Success */

        close(s);
        s=-1;
    }

    if (s<0) {
        complain("di_stream_sock_v6client: could not connect");
        return Err_System;
    }

    if (fcntl(s, F_SETFD, FD_CLOEXEC)<0) {
        complain("di_stream_sock_v6client: fcntl(FD_CLOEXEC): %s",
                strerror(errno));
    }
    if (fcntl(s, F_SETFL, O_NDELAY)<0) {
        *outptr++=errno;
        printf("di_stream_sock_v6client: fcntl(O_NDELAY): %s\n",
                strerror(errno));
        close(s);
        return Err_System;
    }

    private->n_path=s;

    return OK;
}
/*****************************************************************************/
/*
 * connecting socket:
 * init will connect the socket (blocking) and immediately activate reading
 * 
 * listening socket:
 * init will setup a listening socket
 * if a connection is accepted it is immediately activated for reading
 * a broken connection is not fatal
 * 
 * common:
 * as long as readout is not active all data will be discarded
 * if readout is active the data are inserted into the cluster mechanism
 *   as clusterty_async_data2
 * 
 * di_opaque cannot be started or stopped 
 */
/*
 * is start and stop identical with (de)activating readout?
 */
/*
 * possible optimisation:
 *     if data are available but no clusters are free:
 *         either suspend input
 *     or
 *         discard data
 *     (selected by a flag)
 */
errcode
di_stream_sock_init(int idx, __attribute__((unused)) int qlen,
        __attribute__((unused)) ems_u32* q)
{
    struct di_stream_data *private=
            (struct di_stream_data*)datain_cl[idx].private;
    errcode eres;
    size_t i;

    T(readout_em_opaque/di_opaque.c:di_stream_sock_init)

#ifdef DEBUG
    printf("di_stream_sock_init\n");
#endif

    /* verify existence of identifier */
    printf("di_stream_sock_init bufinfosize=%zu\n", datain[idx].bufinfosize);
    for (i=0; i<datain[idx].bufinfosize; i++) {
        printf("%08x ", datain[idx].bufinfo[i]);
    }
    printf("\n\n");
    if (datain[idx].bufinfosize==0) {
        complain("datain stream[%d]: no identifier given", idx);
        return plErr_ArgNum;
    }
    if (datain[idx].bufinfosize!=(datain[idx].bufinfo[0]+3)/4+1) {
        complain("datain stream[%d]: identifier is not a valid string", idx);
        return plErr_ArgRange;
    }

    private=calloc(1, sizeof(struct di_stream_data));
    if (!private) {
        printf("di_cluster_sock_init: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    datain_cl[idx].private=private;

    private->s_path=-1;
    private->n_path=-1;
    private->accepttask=0;
    /* we do not even decode the identifier, we will just copy it verbatim */

    /* decode, modify and encode the identifier */
    /* we (mis)use extractstring to copy the identifier, thus we need
       additional space for the trailing \0 */
    private->idlen=strlen(ID_PREFIX)+datain[idx].bufinfo[0];
    private->idblob=calloc((private->idlen+3)/4+2, 4);
    if (!private->idblob) {
        printf("di_cluster_sock_init: %s\n", strerror(errno));
        free(private);
        datain_cl[idx].private=0;
        return plErr_NoMem;
    }
    private->idblob[0]=ntohl(private->idlen);
    strcpy((char*)(private->idblob+1), ID_PREFIX);
    extractstring((char*)(private->idblob+1)+strlen(ID_PREFIX),
            datain[idx].bufinfo);
    private->idlen=(private->idlen+3)/4+1; /* lengt of ID in words */

    /*  the cluster header needs 10 words:
     *    size, endiantest, type
     *    optionsize, optionflags, timestamp (2 words)
     *    flags, fragment_id, version
     *  and we need space for the identifier and one word for the payload size
     */
    private->maxdatalen=(cluster_max-11-private->idlen)*4;
    private->data=malloc(private->maxdatalen);
    if (!private->data) {
        complain("datain stream[%d]: cannot allocate %d bytes for data",
                idx, private->maxdatalen);
        free(private->idblob);
        free(datain_cl[idx].private);
        datain_cl[idx].private=0;
        return plErr_NoMem;
    }

    datain_cl[idx].procs=sock_procs;
    datain_cl[idx].status=Invoc_notactive;

    switch (datain[idx].addrtyp) {
    case Addr_Socket:
        printf("di_stream_sock_init / Addr_Socket\n");
        private->server=datain[idx].addr.inetsock.host==INADDR_ANY;
        if (private->server)
            eres=di_stream_sock_v4server(idx);
        else
            eres=di_stream_sock_v4client(idx);
        break;
    case Addr_V6Socket:
        printf("di_stream_sock_init / Addr_V6Socket\n");
        private->server=datain[idx].addr.inetV6sock.flags&ip_passive;
        if (private->server)
            eres=di_stream_sock_v6server(idx);
        else
            eres=di_stream_sock_v6client(idx);
        break;
    default:
        complain("di_stream_sock_init: illegal addresstype %d",
            datain[idx].addrtyp);
        eres=Err_ArgRange;
    }

    if (eres!=OK)
        goto error;

    if (private->n_path>=0) {
        union callbackdata data;
        char name[100];
        data.i=idx;
        snprintf(name, 100, "di_%02d_stream_read", idx);
        datain_cl[idx].seltask=sched_insert_select_task(di_stream_read, data,
            name, private->n_path, select_read, 1);
        if (!datain_cl[idx].seltask) {
            datain_cl[idx].error=-1;
            complain("di_stream_sock_client: can't create task.\n");
            close(private->n_path);
            private->n_path=-1;
            goto error;
        }
        //datain_cl[idx].status=Invoc_active;
    }

    return OK;

error:
    free(private->idblob);
    free(datain_cl[idx].private);
    datain_cl[idx].private=0;
    return eres;
}
/*****************************************************************************/
/*****************************************************************************/
