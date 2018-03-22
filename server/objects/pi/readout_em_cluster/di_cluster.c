/*
 * readout_em_cluster/di_cluster.c
 * created 25.03.97 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: di_cluster.c,v 1.21 2017/10/27 21:10:09 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sconf.h>
#include <debug.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "di_cluster.h"
#include "../../../main/scheduler.h"
#include "datains.h"
#include "../../domain/dom_datain.h"
#include "../../../dataout/cluster/do_cluster.h"
#include "../readout.h"
#include "../../../commu/commu.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

#define set_max(a, b) ((a)=(a)<(b)?(b):(a))

struct di_cluster_data {
    /* socket information */
    int server; /* dataout is the passive part */
    int connected;
    int s_path; /* server socket */
    int n_path; /* socket for data */
    struct seltaskdescr* accepttask; /* task for accept (if server) */

    /* data information */
    struct Cluster* cluster;
    int head[2];
    unsigned int idx;
    unsigned int bytes;
};

RCS_REGISTER(cvsid, "objects/pi/readout_em_cluster")

extern ems_u32* outptr;
int ved_info_sent;

/*****************************************************************************/
static void
di_sock_all_done(int idx)
{
struct di_cluster_data *private=(struct di_cluster_data*)datain_cl[idx].private;
T(readout_em_cluster/di_cluster.c:di_sock_all_done)

datain_cl[idx].status=Invoc_alldone;
sched_delete_select_task(datain_cl[idx].seltask);
datain_cl[idx].seltask=0;
close(private->n_path);
private->n_path=-1;
private->connected=0;
check_ro_done(idx);
}
/*****************************************************************************/
static void veds_received(int di_idx)
{
    int i, j, k, nv, ved_num=0, missing=0;
    struct Cluster* is_cluster;
    T(readout_em_cluster/di_cluster.c::veds_received)

    printf("veds_received(di_idx=%d): ", di_idx);
    for (i=0; i<MAX_DATAIN; i++) {
        if (datain[i].bufftyp!=-1) {
            if (datain_cl[i].vedinfos.num_veds>=0) {
                ved_num+=datain_cl[i].vedinfos.num_veds;
            } else {
                if (missing++)
                    printf(", %d", i);
                else
                    printf("datain %d", i);
            }
        }
    }
    if (missing) {
        printf(" fehl%s noch, %d veds bisher.\n", missing==1?"t":"en", ved_num);
        return;
    }

    printf("%d veds.\n", ved_num);

    vedinfos.num_veds=ved_num;
    vedinfos.version=1;
    vedinfos.info=(struct VEDinfo*)malloc(ved_num*sizeof(struct VEDinfo));

    for (i=0, nv=0; i<MAX_DATAIN; i++) {
        if (datain[i].bufftyp!=-1) {
            printf("datain %d, %d veds, version=%d:\n",
                i, datain_cl[i].vedinfos.num_veds,
                datain_cl[i].vedinfos.version);
            set_max(vedinfos.version, datain_cl[i].vedinfos.version); 
            for (j=0; j<datain_cl[i].vedinfos.num_veds; j++) {
                struct VEDinfo* info=datain_cl[i].vedinfos.info+j;
                printf("  id=%d; isnum=%d\n", info->ved_id, info->num_is);
                vedinfos.info[nv]=*info;
                vedinfos.info[nv].is_info=(struct ISinfo*)malloc(info->num_is*sizeof(struct ISinfo));
                for (k=0; k<info->num_is; k++) {
                    printf("    is[%d]:", k);
                    printf(" ID %d: importance=%d decohints=0x%x",
                        info->is_info[k].is_id,
                        info->is_info[k].importance,
                        info->is_info[k].decoding_hints);
                    vedinfos.info[nv].is_info[k].is_id=info->is_info[k].is_id;
                    vedinfos.info[nv].is_info[k].importance=info->is_info[k].importance;
                    vedinfos.info[nv].is_info[k].decoding_hints=info->is_info[k].decoding_hints;
                }
                printf("\n");
                nv++;
            }
        }
    }

    is_cluster=create_ved_cluster();
    if (is_cluster==0) {
        fatal_readout_error();
    } else {
        clusters_deposit(is_cluster);
        printf("ved-cluster deponiert\n");
    }
    ved_info_sent=1;
    outputbuffer_freed();
}
/*****************************************************************************/
static void check_displacement(int evnum)
{
int idx;
T(readout_em_cluster/di_cluster.c:check_displacement)

for (idx=0; idx<MAX_DATAIN; idx++)
  {
  if (datain[idx].bufftyp!=-1)
    {
    Datain_cl* di_cl=datain_cl+idx;
    if (di_cl->max_displacement)
      {
      int i;
      for (i=0; i<di_cl->vedinfos.num_veds; i++)
        {
        if (di_cl->vedinfos.info[i].events+di_cl->max_displacement<evnum)
          {
          /*printf("displacement: di_idx %d, ved %d (orig=%d): events=%d (soll=%d)\n",
              idx,
              di_cl->vedinfos[i].id,
              di_cl->vedinfos[i].orig_id,
              di_cl->vedinfos[i].events,
              evnum);*/
          di_cl->procs.request_(idx, i, evnum);
          }
        }
      }
    }
  }
}
/*****************************************************************************/
static void di_cluster_read(int path, enum select_types selected,
        union callbackdata data)
{
int di_idx=data.i;
int res;
Datain_cl* di_cl=datain_cl+di_idx;
struct di_cluster_data* dat=(struct di_cluster_data*)datain_cl[di_idx].private;
T(readout_em_cluster/di_cluster.c:di_cluster_read)

if (selected!=select_read)
  {
  ems_u32 buf[4];
  printf("objects/pi/readout_em_cluster/di_cluster.c::di_cluster_read():\n"
         "  selected=%d\n", selected);
  di_cl->error=-1;
  di_cl->status=Invoc_error;
  sched_delete_select_task(di_cl->seltask);
  di_cl->seltask=0;
  buf[0]=rtErr_InDev;
  buf[1]=di_idx;
  buf[2]=2;
  buf[3]=selected;
  send_unsolicited(Unsol_RuntimeError, buf, 4);
  fatal_readout_error();
  return;
  }
if (dat->cluster==0) /* noch kein Cluster alloziert */
  {
  if (dat->idx<2*sizeof(int)) /* header noch nicht vollstaendig */
    {
    res=read(path, ((char*)dat->head)+dat->idx, 2*sizeof(int)-dat->idx);
    if (res<=0) /* Fehler? */
      {
      if (res==0) errno=EPIPE;
      printf("di_cluster_read_a(path=%d, di_idx=%d, idx=%d): %s\n", path,
          di_idx, dat->idx, strerror(errno));
      if ((errno!=EINTR) && (errno!=EWOULDBLOCK))
        {
        ems_u32 buf[4];
        di_cl->error=errno;
        di_cl->status=Invoc_error;
        sched_delete_select_task(di_cl->seltask);
        di_cl->seltask=0;
        buf[0]=rtErr_InDev;
        buf[1]=di_idx;
        buf[2]=3;
        buf[3]=errno;
        send_unsolicited(Unsol_RuntimeError, buf, 4);
        fatal_readout_error();
        }
      return;
      }
    dat->idx+=res;
    }
  if (dat->idx==2*sizeof(int)) /* header vollstaendig */
    {
    size_t size;
    int swapped;

    swapped=dat->head[1]!=0x12345678;
    if (swapped)
      {
      if (dat->head[1]!=0x78563412)
        {
        ems_u32 buf[4];
        printf("di_cluster_read: wrong endien: 0x%08X\n", dat->head[1]);
        di_cl->error=-1;
        di_cl->status=Invoc_error;
        sched_delete_select_task(di_cl->seltask);
        di_cl->seltask=0;
        buf[0]=rtErr_InDev;
        buf[1]=di_idx;
        buf[2]=4;
        buf[3]=dat->head[1];
        send_unsolicited(Unsol_RuntimeError, buf, 4);
        fatal_readout_error();
        return;
        }
      size=swap_int(dat->head[0])+1;
      }
    else
      size=dat->head[0]+1;
    if (size>CLUSTER_MAX) printf("di_cluster.c: JUMBOCLUSTER "
        "(di=%d; size=%zu; CLUSTER_MAX=%llu)\n",
        di_idx, size, CLUSTER_MAX);
    dat->cluster=clusters_create(size, di_idx, "di_cluster_read");
    if (dat->cluster)
      {
      dat->cluster->size=size;
      dat->cluster->swapped=swapped;
      ClLEN(dat->cluster->data)=dat->head[0];
      ClENDIEN(dat->cluster->data)=dat->head[1];
      dat->bytes=(size)*sizeof(int);
      }
    else
      {
      di_cl->suspended=1;
      sched_select_task_suspend(di_cl->seltask);
      di_cl->suspensions++;
      }
    }
  }
else
  {
  char* data=(char*)dat->cluster->data;
  res=read(path, data+dat->idx, dat->bytes-dat->idx);
  if (res<=0) /* Fehler? */
    {
    if (res==0) errno=EPIPE;
    printf("di_cluster_read_b(path=%d, di_idx=%d, idx=%d, dest=%p, size=%d): \n"
        "    %s\n", path, di_idx, dat->idx, data+dat->idx, dat->bytes-dat->idx,
        strerror(errno));
    if ((errno!=EINTR) && (errno!=EWOULDBLOCK))
      {
      ems_u32 buf[4];
      di_cl->error=errno;
      di_cl->status=Invoc_error;
      sched_delete_select_task(di_cl->seltask);
      di_cl->seltask=0;
      buf[0]=rtErr_InDev;
      buf[1]=di_idx;
      buf[2]=5;
      buf[3]=errno;
      send_unsolicited(Unsol_RuntimeError, buf, 4);
      fatal_readout_error();
      }
    return;
    }
  dat->idx+=res;
  if (dat->idx==dat->bytes) /* Cluster vollstaendig gelesen */
    {
    struct Cluster* cluster=dat->cluster;
    if (cluster->swapped)
      {
      cluster->type=swap_int(ClTYPE(cluster->data));
      cluster->optsize=swap_int(ClOPTSIZE(cluster->data));
      if (cluster->type==clusterty_events) /* enthaelt Eventdaten */
        {
        cluster->flags=swap_int(ClsFLAGS(cluster->data, cluster->optsize));
        cluster->ved_id=swap_int(ClsVEDID(cluster->data, cluster->optsize));
        cluster->ved_id=(cluster->ved_id<<8)+di_idx;
        ClsVEDID(cluster->data, cluster->optsize)=swap_int(cluster->ved_id);
        cluster->numevents=swap_int(ClsEVNUM(cluster->data, cluster->optsize));
        if (cluster->numevents)
          cluster->firstevent=swap_int(ClsEVID1(cluster->data, cluster->optsize));
        else
          cluster->firstevent=0;
        }
      }
    else
      {
      cluster->type=ClTYPE(cluster->data);
      cluster->optsize=ClOPTSIZE(cluster->data);
      if (cluster->type==clusterty_events) /* enthaelt Eventdaten */
        {
        cluster->flags=ClFLAGS(cluster->data);
        cluster->ved_id=(ClVEDID(cluster->data)<<8)+di_idx;
        ClVEDID(cluster->data)=cluster->ved_id;
        cluster->numevents=ClEVNUM(cluster->data);
        if (cluster->numevents)
          cluster->firstevent=ClEVID1(cluster->data);
        else
          cluster->firstevent=0;
        }
      }
    if (cluster->type==clusterty_no_more_data)
      {
      printf("di %d: no_more_data\n", di_idx);
      if (di_cl->vedinfos.num_veds<=0)
        printf("di %d: received no_more_data with num_veds=%d\n",
            di_idx, di_cl->vedinfos.num_veds);
      clusters_recycle(cluster);
      dat->cluster=0;
      dat->idx=0;
      di_sock_all_done(di_idx);
      }
    else if (cluster->type==clusterty_ved_info)
      {
      int i, version;
      if (cluster->swapped)
        for (i=0; i<cluster->size; i++)
            cluster->data[i]=swap_int(cluster->data[i]);
      if ((cluster->data[4+cluster->optsize]&0x80000000)==0)
        {
        ems_u32 buf[3];
        printf("datain %d: connected server sends old style vedinfo\n", di_idx);
        di_cl->error=-1;
        di_cl->status=Invoc_error;
        sched_delete_select_task(di_cl->seltask);
        di_cl->seltask=0;
        buf[0]=rtErr_InDev;
        buf[1]=di_idx;
        buf[2]=6;
        send_unsolicited(Unsol_RuntimeError, buf, 3);
        fatal_readout_error();
        return;
        }
      version=cluster->data[4+cluster->optsize]&0x7fffffff;
      switch (version)
        {
        case 1: /* nobreak */
        case 2:
          {
          unsigned int* p;
          int i;
          if (di_cl->vedinfos.info)
            {
            for (i=0; i<di_cl->vedinfos.num_veds; i++)
              free(di_cl->vedinfos.info[i].is_info);
            free(di_cl->vedinfos.info);
            }
          p=cluster->data+5+cluster->optsize;
          set_max(di_cl->vedinfos.version, version);
          di_cl->vedinfos.num_veds=*p++;
          di_cl->vedinfos.info=malloc(di_cl->vedinfos.num_veds*sizeof(struct VEDinfo));
          for (i=0; i<di_cl->vedinfos.num_veds; i++)
            {
            int j, importance=0;
            struct VEDinfo* info=di_cl->vedinfos.info+i;
            info->orig_id=*p++;
            info->ved_id=(info->orig_id<<8)+di_idx;
            if (version==2)
                importance=*p++;
            info->num_is=*p++;
            info->is_info=(struct ISinfo*)malloc(info->num_is*sizeof(struct ISinfo));
            for (j=0; j<info->num_is; j++) {
                info->is_info[j].is_id=*p++;
                info->is_info[j].importance=importance;
                info->is_info[j].decoding_hints=0;
            }
            info->events=0;
            }
          }
          break;
        case 3: {
            unsigned int* p;
            int i;
            if (di_cl->vedinfos.info) {
                for (i=0; i<di_cl->vedinfos.num_veds; i++)
                    free(di_cl->vedinfos.info[i].is_info);
                free(di_cl->vedinfos.info);
            }
            p=cluster->data+5+cluster->optsize;
            set_max(di_cl->vedinfos.version, version);
            di_cl->vedinfos.num_veds=*p++;
            di_cl->vedinfos.info=malloc(di_cl->vedinfos.num_veds*sizeof(struct VEDinfo));
            for (i=0; i<di_cl->vedinfos.num_veds; i++) {
                int j;
                struct VEDinfo* info=di_cl->vedinfos.info+i;
                info->orig_id=*p++;
                info->ved_id=(info->orig_id<<8)+di_idx;
                info->num_is=*p++;
                info->is_info=(struct ISinfo*)malloc(info->num_is*sizeof(struct ISinfo));
                for (j=0; j<info->num_is; j++) {
                    info->is_info[j].is_id=*p++;
                    info->is_info[j].importance=*p++;
                    info->is_info[j].decoding_hints=*p++;
                }
                info->events=0;
            }
          }
          break;
        default:
          {
          ems_u32 buf[4];
          printf("datain %d: connected server sends vedinfo type %d\n",
              di_idx, cluster->data[4+cluster->optsize]&0x7fffffff);
          di_cl->error=-1;
          di_cl->status=Invoc_error;
          sched_delete_select_task(di_cl->seltask);
          di_cl->seltask=0;
          buf[0]=rtErr_InDev;
          buf[1]=di_idx;
          buf[2]=7;
          buf[3]=cluster->data[4+cluster->optsize]&0x7fffffff;
          send_unsolicited(Unsol_RuntimeError, buf, 4);
          fatal_readout_error();
          }
          return;
        }
      clusters_recycle(cluster);
      dat->cluster=0;
      dat->idx=0;
      {
          int i;
          printf("received ved_info: num_ved=%d\n", di_cl->vedinfos.num_veds);
          for (i=0; i<di_cl->vedinfos.num_veds; i++) {
                int j;
                struct VEDinfo* info=di_cl->vedinfos.info+i;
                printf("Id=%d, %d ISs:", info->ved_id, info->num_is);
                for (j=0; j<info->num_is; j++) {
                    printf("  id=%d importance=%d hints=0x%x\n",
                        info->is_info[j].is_id,
                        info->is_info[j].importance,
                        info->is_info[j].decoding_hints);
                }
          }
      }
      di_cl->suspended=1;
      sched_select_task_suspend(di_cl->seltask);
      veds_received(di_idx);
      }
    else
      {
      if (di_cl->vedinfos.num_veds<=0)
        printf("di %d: received cluster typ %d with num_veds=%d\n",
            di_idx, cluster->type, di_cl->vedinfos.num_veds);
      if (cluster->type!=clusterty_events)
        {
        printf("datain %d: type=%d\n", di_idx, cluster->type);
        }
      else
        {
        int n;
        int evnum=cluster->firstevent+cluster->numevents-1;
        /*
        if (cluster->firstevent!=di_cl->eventcnt+1)
          {
          printf("datain %d: last event=%d; new event=%d\n", di_idx,
              di_cl->eventcnt, cluster->firstevent);
          }*/
        for (n=0; n<di_cl->vedinfos.num_veds; n++)
          {
          if (di_cl->vedinfos.info[n].ved_id==cluster->ved_id)
            {
#if 0
            if (cluster->firstevent!=di_cl->vedinfos.info[n].events+1)
              {
              printf("datain %d, ved %d: jump from event %d to %d\n",
                  di_idx, di_cl->vedinfos.info[n].orig_id,
                  di_cl->vedinfos.info[n].events, cluster->firstevent);
              }
#endif
            di_cl->vedinfos.info[n].events=evnum;
            break;
            }
          }
        check_displacement(evnum);
        }
      di_cl->clusters++;
      di_cl->words+=cluster->size;
      clusters_deposit(cluster);
      dat->cluster=0;
      dat->idx=0;
      }
    }
  }
}

/*****************************************************************************/
static void
di_sock_connect(int path, enum select_types selected,
        union callbackdata data)
{
int idx=data.i;
struct di_cluster_data *private=(struct di_cluster_data*)datain_cl[idx].private;
T(readout_em_cluster/di_cluster.c:di_sock_connect)

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
  fatal_readout_error();
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
    fatal_readout_error();
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
      fatal_readout_error();
      }
    else /* ok */
      {
      char name[100];
      printf("datain %d connected\n", idx);
      private->connected=1;
      snprintf(name, 100, "di_%02d_cluster_read", idx);
      datain_cl[idx].seltask=sched_insert_select_task(di_cluster_read, data,
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
        fatal_readout_error();
        }
      }
    }
  }
}
/*****************************************************************************/
static void
di_sock_accept(int path, enum select_types selected,
        union callbackdata data)
{
int idx=data.i;
struct di_cluster_data *private=(struct di_cluster_data*)datain_cl[idx].private;
int newpath;
T(readout_em_cluster/di_cluster.c:di_sock_accept)

if (selected&select_except)
  {
  if (!private->connected)
    {
    ems_u32 buf[3];
    datain_cl[idx].error=-1;
    close(path);
    private->s_path=-1;
    sched_delete_select_task(private->accepttask);
    private->accepttask=0;
    datain_cl[idx].status=Invoc_error;
    buf[0]=rtErr_InDev;
    buf[1]=idx;
    buf[2]=12;
    send_unsolicited(Unsol_RuntimeError, buf, 3);
    fatal_readout_error();
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
      fatal_readout_error();
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
        printf("di_cluster.c: fcntl(newpath, FD_CLOEXEC): %s\n", strerror(errno));
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
        fatal_readout_error();
        }
      else /* fcntl ok */
        {
        if (datain_cl[idx].status==Invoc_active)
          {
          char name[100];
          snprintf(name, 100, "di_%02d_cluster_read", idx);
          datain_cl[idx].seltask=sched_insert_select_task(di_cluster_read, data,
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
            fatal_readout_error();
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
/*****************************************************************************/
static errcode
di_cluster_sock_start(int idx)
{
struct di_cluster_data *private=(struct di_cluster_data*)datain_cl[idx].private;
union callbackdata data;
data.i=idx;
T(readout_em_cluster/di_cluster.c:di_cluster_sock_start)

if (!private->server)
  {
  struct sockaddr_in sa;
  int r;
  
  private->n_path=socket(AF_INET, SOCK_STREAM, 0);
  if (private->n_path<0)
    {
    *outptr++=errno;
    printf("di_cluster_sock_start: can't open socket: %s\n", strerror(errno));
    return Err_System;
    }
  if (fcntl(private->n_path, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("di_cluster.c: fcntl(private->n_path, FD_CLOEXEC): %s\n", strerror(errno));
    }

  if (fcntl(private->n_path, F_SETFL, O_NDELAY)<0)
    {
    *outptr++=errno;
    datain_cl[idx].error=errno;
    printf("di_cluster_sock_start: can't set to O_NDELAY: %s\n",
        strerror(errno));
    return Err_System;
    }

  sa.sin_family=AF_INET;
  sa.sin_port=htons(datain[idx].addr.inetsock.port);
  sa.sin_addr.s_addr=htonl(datain[idx].addr.inetsock.host);
  r=connect(private->n_path, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
  if (r==0) /* immediately connected */
    {
    char name[100];
    snprintf(name, 100, "di_%02d_cluster_read", idx);
    datain_cl[idx].seltask=sched_insert_select_task(di_cluster_read, data,
        name, private->n_path, select_read|select_except, 1);
    if (datain_cl[idx].seltask==0)
      {
      datain_cl[idx].error=-1;
      printf("di_cluster_sock_start: can't create task.\n");
      return Err_System;
      }
    private->connected=1;
    }
  else if (errno==EINPROGRESS)
    {
    char name[100];
    snprintf(name, 100, "di_%02d_sock_connect", idx);
    datain_cl[idx].seltask=sched_insert_select_task(di_sock_connect, data,
        name, private->n_path, select_write|select_except, 1);
    if (datain_cl[idx].seltask==0)
      {
      datain_cl[idx].error=-1;
      printf("di_cluster_sock_start: can't create task.\n");
      return Err_System;
      }
    }
  else /* Fehler */
    {
    *outptr++=errno;
    datain_cl[idx].error=errno;
    printf("di_cluster_sock_start: can't connect socket: %s\n",
        strerror(errno));
    return Err_System;
    }
  }
else /*server */
  {
  if (private->connected)
    {
    char name[100];
    snprintf(name, 100, "di_%02d_cluster_read", idx);
    datain_cl[idx].seltask=sched_insert_select_task(di_cluster_read, data,
        name, private->n_path, select_read|select_except, 1);
    if (datain_cl[idx].seltask==0)
      {
      datain_cl[idx].error=-1;
      printf("di_sock_start: can't create task.\n");
      return Err_System;
      }
    }
  }

datain_cl[idx].suspended=0;
private->cluster=0;
private->idx=0;

/* initialize statistics */
datain_cl[idx].clusters=0;
datain_cl[idx].words=0;
datain_cl[idx].suspensions=0;

/* possible memory leak? */
if (datain_cl[idx].vedinfos.info!=0)
  {
  printf("datain %d: vedinfos.info=%p, num_veds=%d\n", idx,
      datain_cl[idx].vedinfos.info, datain_cl[idx].vedinfos.num_veds);
  }
datain_cl[idx].vedinfos.info=0;
datain_cl[idx].vedinfos.num_veds=-1;

datain_cl[idx].status=Invoc_active;
return OK;
}
/*****************************************************************************/
static errcode
di_cluster_sock_stop(int idx)
{
T(readout_em_cluster/di_cluster.c:di_cluster_sock_stop)
if (datain_cl[idx].status!=Invoc_active)
  {
  printf("di_cluster_sock_stop(%d): status=%d\n", idx, datain_cl[idx].status);
  }
sched_select_task_suspend(datain_cl[idx].seltask);
datain_cl[idx].status=Invoc_stopped;
return OK;
}
/*****************************************************************************/
static errcode
di_cluster_sock_resume(int idx)
{
T(readout_em_cluster/di_cluster.c:di_cluster_sock_resume)

if (datain_cl[idx].status!=Invoc_stopped)
  {
  printf("di_cluster_sock_resume(%d): status=%d\n", idx, datain_cl[idx].status);
  return Err_Program;
  }
if (datain_cl[idx].seltask==0)
  {
  printf("di_cluster_sock_resume(%d): seltask==0\n", idx);
  return Err_Program;
  }
sched_select_task_reactivate(datain_cl[idx].seltask);
datain_cl[idx].status=Invoc_active;
return OK;
}
/*****************************************************************************/
static errcode
di_cluster_sock_reset(int idx)
{
struct di_cluster_data *private=(struct di_cluster_data*)datain_cl[idx].private;
T(readout_em_cluster/di_cluster.c:di_cluster_sock_reset)

if (datain_cl[idx].status==Invoc_notactive)
  {
  printf("di_cluster_sock_reset(%d): not active\n", idx);
  return Err_Program;
  }

if (private->n_path>=0) {close(private->n_path); private->n_path=-1;}
private->connected=0;
datain_cl[idx].error=0;
sched_delete_select_task(datain_cl[idx].seltask);
datain_cl[idx].seltask=0;

datain_cl[idx].status=Invoc_notactive;
if (datain_cl[idx].vedinfos.info)
  {
  int i;
  for (i=0; i<datain_cl[idx].vedinfos.num_veds; i++)
    {
    free(datain_cl[idx].vedinfos.info[i].is_info);
    }
  free(datain_cl[idx].vedinfos.info);
  }
datain_cl[idx].vedinfos.info=0;
datain_cl[idx].vedinfos.num_veds=-1;
return OK;
}
/*****************************************************************************/
static void
di_cluster_sock_reactivate(int idx)
{
T(readout_em_cluster/di_cluster.c:di_cluster_sock_reactivate)

if (datain_cl[idx].seltask)
  {
  sched_select_task_reactivate(datain_cl[idx].seltask);
  datain_cl[idx].suspended=0;
  }
else
  printf("di_cluster_sock_reactivate(idx=%d): no task\n", idx);
}
/*****************************************************************************/
static void
di_cluster_sock_cleanup(int idx)
{
struct di_cluster_data *private=(struct di_cluster_data*)datain_cl[idx].private;
T(readout_em_cluster/di_cluster.c:di_cluster_sock_cleanup)

printf("di_cluster_sock_cleanup(%d)\n", idx);
if (private->s_path>=0) close(private->s_path);
if (private->n_path>=0) close(private->n_path);
sched_delete_select_task(private->accepttask);
if (datain_cl[idx].seltask)
    printf("Fehler in di_cluster_sock_cleanup: seltask!=0\n");
sched_delete_select_task(datain_cl[idx].seltask);
free(datain_cl[idx].private);
datain_cl[idx].private=0;
}
/*****************************************************************************/
static int xsend(int path, char* data, int size)
{
int da=0; int res;
do
  {
  res=write(path, data+da, size-da);
  if (res>0)
    da+=res;
  else if ((res<0) && (errno!=EINTR))
    {
    printf("readout_em_cluster/di_cluster.c:xsend: %s\n", strerror(errno));
    return -1;
    }
  }
while (da<size);
return 0;
}
/*****************************************************************************/
static void
di_cluster_sock_request(int di_idx, int ved_idx, int evnum)
{
Datain_cl* di_cl=datain_cl+di_idx;
struct di_cluster_data *private=
        (struct di_cluster_data*)datain_cl[di_idx].private;
int req[4], i;
/*printf("di_cluster_sock_request(di_idx=%d, ved_idx=%d, evnum=%d)\n",
  di_idx, ved_idx, evnum);*/
/*
req[0]: size ==3 
req[1]: code ==1
req[2]: ved_id
req[3]: evnum
*/

T(readout_em_cluster/di_cluster.c:di_cluster_sock_request)

/* printf("di_cluster_sock_request(%d, %d, %d)\n", di_idx, ved_idx, evnum); */
req[0]=3;
req[1]=1;
req[2]=di_cl->vedinfos.info[ved_idx].orig_id;
req[3]=evnum;
for (i=0; i<4; i++) req[i]=htonl(req[i]);
xsend(private->n_path, (char*)req, 4*sizeof(int));
}
/*****************************************************************************/
static di_procs sock_procs={
  di_cluster_sock_start,
  di_cluster_sock_stop,
  di_cluster_sock_resume,
  di_cluster_sock_reset,
  di_cluster_sock_cleanup,
  di_cluster_sock_reactivate,
  di_cluster_sock_request
  };
/*****************************************************************************/
errcode
di_cluster_sock_init(int idx, int qlen, ems_u32* q)
{
struct di_cluster_data *private;
union callbackdata data;
data.i=idx;
T(readout_em_cluster/di_cluster.c:di_cluster_sock_init)
printf("di_cluster_sock_init(idx=%d, qlen=%d, q=%p)\n", idx, qlen, q);

private=calloc(1, sizeof(struct di_cluster_data));
if (!private) {
    printf("di_cluster_sock_init: %s\n", strerror(errno));
    return plErr_NoMem;
}
datain_cl[idx].private=private;

datain_cl[idx].procs=sock_procs;
datain_cl[idx].seltask=0;
datain_cl[idx].suspended=0;
datain_cl[idx].status=Invoc_notactive;
datain_cl[idx].error=0;
#if 0
datain_cl[idx].cluster_num=qlen>0?q[0]:0;
datain_cl[idx].cluster_max=qlen>1?q[1]:0;
#endif
datain_cl[idx].max_displacement=qlen>2?q[2]:0;
#if 0
printf("datain %d: cluster_num=%d\n", idx, datain_cl[idx].cluster_num);
printf("datain %d: cluster_max=%d\n", idx, datain_cl[idx].cluster_max);
#endif
printf("datain %d: max_displacement=%d\n", idx, datain_cl[idx].max_displacement);
{
struct in_addr addr;
addr.s_addr=htonl(datain[idx].addr.inetsock.host);
printf("host=%s\n", inet_ntoa(addr));
printf("port=%d\n", datain[idx].addr.inetsock.port);
}
private->server=datain[idx].addr.inetsock.host==INADDR_ANY;
private->connected=0;
private->s_path=-1;
private->n_path=-1;
private->accepttask=0;

if (private->server)
  {
  struct sockaddr_in sa;
  char name[100];

  private->s_path=socket(AF_INET, SOCK_STREAM, 0);
  if (private->s_path<0)
    {
    *outptr++=errno;
    printf("di_cluster_sock_init: can't open socket: %s\n", strerror(errno));
    return Err_System;
    }
  if (fcntl(private->s_path, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("di_cluster.c: fcntl(private->s_path, FD_CLOEXEC): %s\n", strerror(errno));
    }
  
  sa.sin_family=AF_INET;
  sa.sin_port=htons(datain[idx].addr.inetsock.port);
  sa.sin_addr.s_addr=INADDR_ANY;
  if(bind(private->s_path, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))<0)
    {
    *outptr++=errno;
    printf("di_cluster_sock_init: can't bind socket: %s\n", strerror(errno));
    close(private->s_path);
    return Err_System;
    }
  if (fcntl(private->s_path, F_SETFL, O_NDELAY)<0)
    {
    *outptr++=errno;
    printf("di_cluster_sock_init: can't set to O_NDELAY: %s\n",
        strerror(errno));
    close(private->s_path);
    return Err_System;
    }
  if (listen(private->s_path, 1)<0)
    {
    *outptr++=errno;
    printf("di_cluster_sock_init: can't listen: %s\n", strerror(errno));
    close(private->s_path);
    return Err_System;
    }

  snprintf(name, 100, "di_%02d_sock_accept", idx);
  private->accepttask=sched_insert_select_task(di_sock_accept, data,
      name, private->s_path, select_read|select_except, 1);
  if (private->accepttask==0)
    {
    printf("di_cluster_sock_init: can't create task.\n");
    close(private->s_path);
    return Err_System;
    }
  }
return OK;
}
/*****************************************************************************/
/*****************************************************************************/
