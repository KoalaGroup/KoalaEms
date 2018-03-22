/*
 * dataout/cluster/do_cluster_sock.c
 * created      14.04.97 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_cluster_sock.c,v 1.15 2017/10/20 23:21:31 wuestner Exp $";

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sconf.h>
#include <debug.h>
#include <emssocktypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "do_cluster.h"
#include "../../main/scheduler.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/pi/readout.h"
#include "../../commu/commu.h"

struct do_special_sock {
    int server; /* dataout is the passive part */
    int connected;
    int s_path; /* server socket */
    int n_path; /* socket for data */
    struct seltaskdescr* connecttask; /* task for connect or accept */
};

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
static void
do_sock_connect(int path, enum select_types selected, union callbackdata data)
{
int do_idx=data.i;
struct do_special_sock* special=(struct do_special_sock*)dataout_cl[do_idx].s;
T(dataout/cluster/do_cluster_sock.c:do_sock_connect)

sched_delete_select_task(special->connecttask);
special->connecttask=0;

if (selected&select_except)
  {
  printf("Error in do_sock_connect(path=%d; do_idx=%d)\n", path, data.i);
  dataout_cl[do_idx].errorcode=-1;
  dataout_cl[do_idx].do_status=Do_error;
  }
else
  {
  ems_socklen_t ilen;
  int val;
  ilen=sizeof(int);
  if (getsockopt(path, SOL_SOCKET, SO_ERROR, (char*)&val, &ilen)<0)
    {
    dataout_cl[do_idx].errorcode=errno;
    dataout_cl[do_idx].do_status=Do_error;
    printf("Error in do_sock_connect(do_idx=%d)::getsockopt: %s\n", data.i,
        strerror(errno));
    if (dataout[do_idx].wieviel==0)
      {
      ems_u32 buf[4];
      buf[0]=rtErr_OutDev;
      buf[1]=do_idx;
      buf[2]=0;
      buf[3]=errno;
      send_unsolicited(Unsol_RuntimeError, buf, 4);
      fatal_readout_error();
      }
    else
      printf("Error ignored\n");
    }
  else
    {
    if (val!=0)
      {
      dataout_cl[do_idx].errorcode=val;
      dataout_cl[do_idx].do_status=Do_error;
      printf("Bad status in do_sock_connect(path=%d; do_idx=%d): %s\n",
          path, data.i, strerror(val));
      if (dataout[do_idx].wieviel==0)
        {
        ems_u32 buf[4];
        buf[0]=rtErr_OutDev;
        buf[1]=do_idx;
        buf[2]=0;
        buf[3]=val;
        send_unsolicited(Unsol_RuntimeError, buf, 4);
        fatal_readout_error();
        }
      else
        printf("Error ignored\n");
      }
    else /* ok */
      {
      char name[100];
      printf("dataout %d connected\n", data.i);
      special->connected=1;
      snprintf(name, 100, "do_%02d_cluster_write", do_idx);
      dataout_cl[do_idx].seltask=sched_insert_select_task(do_cluster_write,
          data, name, path,
          select_write|select_read|select_except
#ifdef SELECT_STATIST
          , 1
#endif
          );
      if (dataout_cl[do_idx].seltask==0)
        {
        dataout_cl[do_idx].errorcode=-1;
        dataout_cl[do_idx].do_status=Do_error;
        printf("do_sock_connect: can't create task.\n");
        fatal_readout_error();
        }
      }
    }
  }
}
/*****************************************************************************/
static void
do_sock_accept(int path, enum select_types selected, union callbackdata data)
{
    int do_idx=data.i;
    struct do_special_sock* special=
            (struct do_special_sock*)dataout_cl[do_idx].s;
    int newpath;

    T(cluster/do_cluster_sock.c:do_sock_accept)

    if (selected&select_except) {
        dataout_cl[do_idx].errorcode=-1;
        close(path);
        special->s_path=-1;
        sched_delete_select_task(special->connecttask);
        special->connecttask=0;
        dataout_cl[do_idx].do_status=Do_error;
        printf("Error in do_sock_accept.\n");
    } else {
        struct sockaddr addr;
        ems_socklen_t size;
        size=sizeof(struct sockaddr);
        newpath=accept(path, &addr, &size);
        if (newpath<0) {
            printf("do_sock_accept(do_idx=%d): error: %s\n",
                data.i, strerror(errno));
            dataout_cl[do_idx].errorcode=-1;
        } else { /* accept ok */
            if (special->connected) {
                printf("do_sock_accept(do_idx=%d): already used.\n", do_idx);
                close(newpath);
            } else {
                printf("dataout %d accepted\n", data.i);
                if (fcntl(newpath, F_SETFD, FD_CLOEXEC)<0) {
                    printf("do_cluster_sock.c: "
                            "fcntl(newpath, FD_CLOEXEC): %s\n",
                        strerror(errno));
                }

                special->n_path=newpath;
                if (fcntl(special->n_path, F_SETFL, O_NDELAY)<0) {
                    dataout_cl[do_idx].errorcode=-1;
                    dataout_cl[do_idx].do_status=Do_error;
                    printf("do_sock_accept(do_idx=%d): "
                            "can't set to O_NDELAY: %s\n",
                        data.i,
                    strerror(errno));
                } else { /* fcntl ok */
                    if (dataout_cl[do_idx].do_status==Do_running) {
                        char name[100];
                        snprintf(name, 100, "do_%02d_cluster_write", do_idx);
                        dataout_cl[do_idx].seltask=
                            sched_insert_select_task(do_cluster_write,
                            data, name, special->n_path,
                            select_write|select_read|select_except
#ifdef SELECT_STATIST
                            , 1
#endif
                            );
                        printf("do_sock_accept(do_idx=%d): "
                                "do_cluster_write inserted "
                            "in scheduler\n", do_idx);
                        if (dataout_cl[do_idx].seltask==0) {
                            dataout_cl[do_idx].errorcode=-1;
                            dataout_cl[do_idx].do_status=Do_error;
                            printf("do_sock_accept(do_idx=%d): "
                                    "can't create task.\n", data.i);
                        } else { /* task ok */
                            special->connected=1;
                            if (!dataout_cl[do_idx].vedinfo_sent)
                                send_ved_cluster(do_idx);
                        }
                    } else { /* !Do_running */
                        printf("do_status: not running\n");
                        special->connected=1;
                    }
                }
            }
        }
    }
}
/*****************************************************************************/
static errcode do_cluster_sock_start(int do_idx)
{
/*
called from start readout
*/
struct do_special_sock* special=(struct do_special_sock*)dataout_cl[do_idx].s;
struct do_cluster_data* dat=&dataout_cl[do_idx].d.c_data;
union callbackdata data;
data.i=do_idx;

T(cluster/do_cluster_sock.c:do_cluster_sock_start)
printf("do_cluster_sock_start; do_idx=%d\n", do_idx);
if (!special->server)
  {
  struct sockaddr_in sa;
  int r;
  
  special->n_path=socket(AF_INET, SOCK_STREAM, 0);
  if (special->n_path<0)
    {
    *outptr++=errno;
    dataout_cl[do_idx].do_status=Do_error;
    printf("do_cluster_sock_start(do_idx=%d): can't open socket: %s\n", do_idx,
        strerror(errno));
    return Err_System;
    }
  if (fcntl(special->n_path, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("do_cluster_sock.c: fcntl(special->n_path, FD_CLOEXEC): %s\n",
            strerror(errno));
    }
  if (fcntl(special->n_path, F_SETFL, O_NDELAY)<0)
    {
    *outptr++=errno;
    dataout_cl[do_idx].errorcode=errno;
    dataout_cl[do_idx].do_status=Do_error;
    printf("do_cluster_sock_start(do_idx=%d): can't set to O_NDELAY: %s\n",
        do_idx, strerror(errno));
    return Err_System;
    }

  sa.sin_family=AF_INET;
  sa.sin_port=htons(dataout[do_idx].addr.inetsock.port);
  sa.sin_addr.s_addr=htonl(dataout[do_idx].addr.inetsock.host);
  r=connect(special->n_path, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
  if (r==0)
    special->connected=1;
  else if (errno==EINPROGRESS)
    {
    char name[100];
    snprintf(name, 100, "do_%02d_sock_connect", do_idx);
    special->connecttask=sched_insert_select_task(do_sock_connect, data,
        name, special->n_path, select_write|select_except
#ifdef SELECT_STATIST
        , 1
#endif
        );
    if (special->connecttask==0)
      {
      dataout_cl[do_idx].errorcode=-1;
      dataout_cl[do_idx].do_status=Do_error;
      printf("do_cluster_sock_start(do_idx=%d): can't create task.\n", do_idx);
      return Err_System;
      }
    }
  else
    {
    *outptr++=errno;
    dataout_cl[do_idx].errorcode=errno;
    dataout_cl[do_idx].do_status=Do_error;
    printf("do_cluster_sock_start(do_idx=%d): can't connect socket: %s\n",
        do_idx, strerror(errno));
    return Err_System;
    }
  }

if (special->connected) /* egal, ob client oder server */
  {
  char name[100];
  snprintf(name, 100, "do_%02d_cluster_write", do_idx);
  dataout_cl[do_idx].seltask=sched_insert_select_task(do_cluster_write, data,
      name, special->n_path,
      select_write|select_read|select_except
#ifdef SELECT_STATIST
      , 1
#endif
      );
  if (dataout_cl[do_idx].seltask==0)
    {
    dataout_cl[do_idx].errorcode=-1;
    dataout_cl[do_idx].do_status=Do_error;
    printf("do_sock_start(do_idx=%d): can't create task.\n", do_idx);
    return Err_System;
    }
  }
else
  dataout_cl[do_idx].seltask=0;

dataout_cl[do_idx].suspended=0;
dataout_cl[do_idx].vedinfo_sent=0;

do_statist[do_idx].clusters=0;
do_statist[do_idx].words=0;
do_statist[do_idx].events=0;
do_statist[do_idx].suspensions=0;

dat->active_cluster=0;
dataout_cl[do_idx].advertised_cluster=0;

if (!special->server || special->connected)
    dataout_cl[do_idx].do_status=Do_running;

return OK;
}
/*****************************************************************************/
static void do_cluster_sock_freeze(int do_idx)
{
/*
called from fatal_readout_error()
stops all activities of this dataout, but leaves all other status unchanged
*/
T(cluster/do_cluster_sock.c:do_cluster_sock_freeze)
D(D_TRIG, printf("do_cluster_sock_freeze(do_idx=%d)\n", do_idx);)
sched_delete_select_task(dataout_cl[do_idx].seltask);
dataout_cl[do_idx].seltask=0;
}
/*****************************************************************************/
static errcode do_cluster_sock_reset(int do_idx)
{
struct do_special_sock* special=(struct do_special_sock*)dataout_cl[do_idx].s;

T(cluster/do_cluster_sock.c:do_cluster_sock_reset)
dataout_cl[do_idx].errorcode=0;
if (dataout_cl[do_idx].do_status!=Do_done)
    dataout_cl[do_idx].do_status=Do_neverstarted;
sched_delete_select_task(dataout_cl[do_idx].seltask);
dataout_cl[do_idx].seltask=0;

if (special->n_path>=0)
  {
  struct linger ling;
  ling.l_onoff=1;
  ling.l_linger=120;
  if (setsockopt(special->n_path, SOL_SOCKET, SO_LINGER, &ling, 
      sizeof(struct linger))<0)
    {
    printf("setsockopt(LINGER): %s\n", strerror(errno));
    }
  close(special->n_path);
  special->n_path=-1;
  }
special->connected=0;
return OK;
}
/*****************************************************************************/
static void do_cluster_sock_patherror(int do_idx,
        __attribute__((unused)) int error)
{
struct do_special_sock* special=(struct do_special_sock*)dataout_cl[do_idx].s;
T(dataout/cluster/do_cluster_sock.c:do_cluster_sock_patherror)
if (special->n_path>=0)
  {
  close(special->n_path);
  special->n_path=-1;
  }
special->connected=0;
}
/*****************************************************************************/
static void do_cluster_sock_cleanup(int do_idx)
{
    struct do_special_sock* special=
            (struct do_special_sock*)dataout_cl[do_idx].s;

    T(dataout/cluster/do_cluster_sock.c:do_cluster_sock_cleanup)

if (special->s_path>=0) close(special->s_path);
if (special->n_path>=0) close(special->n_path);
if (special->server && special->connecttask)
    sched_delete_select_task(special->connecttask);
if (dataout_cl[do_idx].seltask)
  {
  printf("Fehler in do_cluster_sock_cleanup: seltask!=0\n");
  sched_delete_select_task(dataout_cl[do_idx].seltask);
  }
    free(special);
    dataout_cl[do_idx].s=0;
}
/*****************************************************************************/

static struct do_procs sock_procs={
  do_cluster_sock_start,
  do_cluster_sock_reset,
  do_cluster_sock_freeze,
  do_cluster_advertise,
  do_cluster_sock_patherror,
  do_cluster_sock_cleanup,
  /*wind*/ do_NotImp_err_ii,
  /*status*/ 0,
  /*filename*/ 0,
  };

/*****************************************************************************/
errcode do_cluster_sock_init(int do_idx)
{
struct do_special_sock* special;
union callbackdata data;
data.i=do_idx;
errcode err=OK;

T(dataout/cluster/do_cluster_sock.c:do_cluster_sock_init)

    special=calloc(1, sizeof(struct do_special_sock));
    if (!special) {
        complain("cluster_sock_init: %s", strerror(errno));
        return Err_NoMem;
    }
    dataout_cl[do_idx].s=special;
    special->s_path=-1;

dataout_cl[do_idx].procs=sock_procs;
dataout_cl[do_idx].do_status=Do_neverstarted;
dataout_cl[do_idx].doswitch=
    readout_active==Invoc_notactive?Do_enabled:Do_disabled;

special->server=dataout[do_idx].addr.inetsock.host==INADDR_ANY;
special->s_path=-1;
special->n_path=-1;
special->connected=0;
special->connecttask=0;

if (special->server)
  {
  struct sockaddr_in sa;
  char name[100];

  special->s_path=socket(AF_INET, SOCK_STREAM, 0);
  if (special->s_path<0)
    {
    *outptr++=errno;
    printf("do_cluster_sock_init: can't open socket: %s\n", strerror(errno));
    err=Err_System;
    goto error;
    }
  if (fcntl(special->s_path, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("do_cluster_sock.c: fcntl(special->s_path, FD_CLOEXEC): %s\n",
            strerror(errno));
    }
  sa.sin_family=AF_INET;
  sa.sin_port=htons(dataout[do_idx].addr.inetsock.port);
  sa.sin_addr.s_addr=INADDR_ANY;
  if(bind(special->s_path, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))<0)
    {
    *outptr++=errno;
    printf("do_cluster_sock_init: can't bind socket: %s\n", strerror(errno));
    err=Err_System;
    goto error;
    }
  if (fcntl(special->s_path, F_SETFL, O_NDELAY)<0)
    {
    *outptr++=errno;
    printf("do_cluster_sock_init: can't set to O_NDELAY: %s\n",
        strerror(errno));
    err=Err_System;
    goto error;
    }
  if (listen(special->s_path, 1)<0)
    {
    *outptr++=errno;
    printf("do_cluster_sock_init: can't listen: %s\n", strerror(errno));
    err=Err_System;
    goto error;
    }

  snprintf(name, 100, "do_%02d_sock_accept", do_idx);
  special->connecttask=sched_insert_select_task(do_sock_accept, data,
      "do_sock_accept", special->s_path, select_read|select_except
#ifdef SELECT_STATIST
      , 1
#endif
      );
  if (special->connecttask==0)
    {
    printf("do_cluster_sock_init: can't create task.\n");
    err=Err_System;
    goto error;
    }
  }

    return OK;

error:
    close(special->s_path);
    free(special);
    dataout_cl[do_idx].s=0;

    return err;
}
/*****************************************************************************/
/*****************************************************************************/
