/*
 * dataout/cluster/do_cluster_tape.c
 * created      25.04.97 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_cluster_tape.c,v 1.18 2017/10/21 22:48:42 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sconf.h>
#include <debug.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <actiontypes.h>
#include <rcs_ids.h>
#include "../../main/scheduler.h"
#include "../../main/childhndlr.h"
#include "../../commu/commu.h"
#include "../../objects/pi/readout.h"
#include "../../objects/domain/dom_dataout.h"
#include "do_cluster.h"
#include "handlercom.h"
#include "do_cluster_tape.h"
#include "clusterpool.h"
#include <sys/time.h>

#define MAX_WIND 1000
#define UNLOAD_WIND -10000
#define AUFTAPE_VERSION 2

struct do_special_tape {
    int sockets[2];
    pid_t child;
    int fatal_expected;
    int open;
    int linked;
    int records;
    int endoftape_warning;
};

extern ems_u32* outptr;
static hndlr_msg message;
static int xid[MAX_DATAOUT];
static int lastxid[MAX_DATAOUT];

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
static int msg_xread(int path, int* buf, int size, int do_idx)
{
int res, da=0, csize=size*sizeof(int);
while (da<csize)
  {
  if ((res=read(path, (char*)buf+da, csize-da))>=0)
    da+=res;
  else if (errno!=EINTR)
    {
    printf("do_cluster_tape.c::msg_xread(do_idx=%d): %s\n", do_idx,
        strerror(errno));
    return -1;
    }
  }
return 0;
}
/*****************************************************************************/
static int msg_read(int path, int do_idx)
{
T(cluster/do_cluster_tape:msg_read)
if (msg_xread(path, (int*)(&message), 4, do_idx)<0) return -1;
#ifdef CLUSTERCHECK
if (message.size>100)
  {
  printf("do_cluster_tape.c::msg_read: size (%d) too large\n", message.size);
  return -1;
  }
#endif
if (msg_xread(path, message.data, message.size, do_idx)<0) return -1;
return 0;
}
/*****************************************************************************/
static int msg_write(int path, int do_idx)
{
int res, size, idx;
T(cluster/do_cluster_tape:msg_write)
if (path<0)
  {
  printf("do_cluster_tape.c::msg_write(do_idx=%d): path not valid\n", do_idx);
  return -1;
  }
message.xid=++xid[do_idx];
size=(message.size+4)*sizeof(int);
idx=0;
while (size>idx)
  {
  res=write(path, (char*)&message+idx, size-idx);
  if (res>0)
    idx+=res;
  else if (res==0)
    {
    printf("do_cluster_tape.c::msg_write(do_idx=%d): wrote 0 bytes "
        "(broken pipe?)\n", do_idx);
    printf("size=%d, idx=%d\n", size, idx);
    return -1;
    }
  else if (errno!=EINTR)
    {
    printf("do_cluster_tape.c::msg_write(do_idx=%d): %s\n", do_idx,
      strerror(errno));
    return -1;
    }
  }
return 0;
}
/*****************************************************************************/
static void do_tape_print_error(int id)
{
T(cluster/do_cluster_tape:do_tape_print_error)
printf("do_tape_handle(do_idx=%d): errormessage:\n", id);
switch (message.data[0])
  {
  case hndle_none: printf("  no error.\n"); break;
  case hndle_exec:
    printf("  Exec: %s\n", strerror(message.data[1]));
    break;
  case hndle_args:
    printf("  wrong Arguments.\n");
    break;
  case hndle_version:
    printf("  wrong Version: requested: %d; auftape: %d\n",
        AUFTAPE_VERSION, message.data[1]);
    break;
  case hndle_open:
    printf("  Open: %s\n", strerror(message.data[1]));
    break;
  case hndle_linkdata:
    printf("  Link datamodule: %s\n", strerror(message.data[1]));
    break;
  case hndle_readstatus:
    printf("  Read status: %s\n", strerror(message.data[1]));
    break;
  case hndle_write:
    printf("  Write data (scsi error not decoded)\n");
    break;
  case hndle_protocol:
    printf("  Protocol Error.\n");
    break;
  case hndle_wind:
    printf("  Handler Error: wind.\n");
    break;
  case hndle_rewind:
    printf("  Handler Error: rewind.\n");
    break;
  case hndle_seod:
    printf("  Handler Error: seod.\n");
    break;
  case hndle_unload:
    printf("  Handler Error: unload.\n");
    break;
  case hndle_control:
    printf("  Handler Error: control.\n");
    break;
  case hndle_init_scsi:
    printf("  Handler Error: init_scsi.\n");
    break;
  default:
    printf("  unknown error %d\n", message.data[0]);
    break;
  }
}
/*****************************************************************************/
#if 0
static void do_tape_print_msg(int id)
{
switch (message.code.conf)
  {
  case hndlc_none:
    printf("do_tape_handle(do_idx=%d): null-message\n", id);
    break;
  case hndlc_error:
    do_tape_print_error(id);
    break;
  case hndlc_exit:
    printf("do_tape_handle(do_idx=%d): exit-message\n", id);
    break;
  case hndlc_pid:
    printf("do_tape_handle(do_idx=%d): pid of handler: %d\n", id, message.data[0]);
    break;
  case hndlc_status:
    printf("do_tape_handle(do_idx=%d): status-message\n", id);
    break;
  case hndlc_write:
    printf("do_tape_handle(do_idx=%d) write-message: offset %d\n", id,
        message.data[0]);
    break;
  case hndlc_linkdata:
    printf("do_tape_handle(do_idx=%d) link-message: %s\n", id,
        message.data[0]?"linked":"unlinked");
    break;
  case hndlc_filemark:
    printf("do_tape_handle(do_idx=%d) filemark-message\n", id);
    break;
  case hndlc_reopen:
    printf("do_tape_handle(do_idx=%d) reopen-message\n", id);
    break;
  case hndlc_wind:
    printf("do_tape_handle(do_idx=%d) wind-message\n", id);
    break;
  case hndlc_rewind:
    printf("do_tape_handle(do_idx=%d) rewind-message\n", id);
    break;
  case hndlc_seod:
    printf("do_tape_handle(do_idx=%d) seod-message\n", id);
    break;
  case hndlc_unload:
    printf("do_tape_handle(do_idx=%d) unload-message\n", id);
    break;
  case hndlc_control:
    printf("do_tape_handle(do_idx=%d) control-message\n", id);
    break;
  default:
    printf("do_tape_print_msg: unknown message %d\n", message.code.conf);
    break;
  }
}
#endif
/*****************************************************************************/
static void do_tape_write(int path, struct Cluster* cl,
    struct do_special_tape* special, int do_idx)
{
    T(cluster/do_cluster_tape:do_tape_write)

    if (cl->type==clusterty_no_more_data)
        printf("do_tape_write: no_more_data\n");

    message.size=1;
    message.pid=special->child;
    message.code.req=hndlr_write;
    message.data[0]=clusterpool_offset(cl)/sizeof(ems_u32);
    if (msg_write(path, do_idx)<0) {
        printf("do_tape_write do_idx=%d: msg_write failed\n", do_idx);
    }
    special->records++;
}
/*****************************************************************************/
static void do_tape_handle_message(int do_idx)
{
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
struct do_cluster_data* dat=&dataout_cl[do_idx].d.c_data;
T(cluster/do_cluster_tape:do_tape_handle_message)

/* do_tape_print_msg(data.i); */
if (message.pid!=special->child)
  {
  printf("message from pid %d instead of %d\n", message.pid, special->child);
  }
else
  {
  switch (message.code.conf)
    {
    case hndlc_none:
      printf("do_tape_handle(do_idx=%d) null-message\n", do_idx);
      break;
    case hndlc_error:
      do_tape_print_error(do_idx);
      switch (message.data[0])
        {
        case hndle_none: break;
        case hndle_exec:
        case hndle_write:
          {
          ems_u32 buf[8];
          dataout_cl[do_idx].errorcode=(message.data[2]<<16)+
                                        (message.data[3]<<8)+
                                        message.data[4];
          if ((message.data[2]==0xd) && /* sense key */ 
              (message.data[3]==0) && /* sense code */
              (message.data[4]==2))   /* add. sense code qual. */
            { /* end of tape */
            dataout_cl[do_idx].errorcode=ENOSPC;
            dataout_cl[do_idx].do_status=Do_error;
            }
          else
            {
            dataout_cl[do_idx].errorcode=EIO;
            dataout_cl[do_idx].do_status=Do_error;
            }
          buf[0]=rtErr_OutDev;
          buf[1]=do_idx;
          buf[2]=1;
          buf[3]=message.data[1]; /* error_code */
          buf[4]=message.data[2]; /* sense_key */
          buf[5]=message.data[3]; /* add_sense_code */
          buf[6]=message.data[4]; /* add_sense_code_qual */
          buf[7]=message.data[5]; /* eom */
          if (dataout_cl[do_idx].do_status==Do_error)
            {
            send_unsolicited(Unsol_RuntimeError, buf, 8);
            if (readout_active) fatal_readout_error();
            }
          else
            send_unsolicited(Unsol_RuntimeError, buf, 8);
          }
          break;
        case hndle_init_scsi:
          {
          ems_u32 buf[4];
          printf("do_tape_handle(do_idx=%d) init_scsi failed\n", do_idx);
          dataout_cl[do_idx].errorcode=ENOSPC;
          dataout_cl[do_idx].do_status=Do_error;
          buf[0]=rtErr_OutDev;
          buf[1]=do_idx;
          buf[2]=4;
          buf[3]=message.size>=2?message.data[1]:-1;
          send_unsolicited(Unsol_RuntimeError, buf, 4);         
          }
          break;
#if 0
        default: /* sehr fragwuerdig!! */
          {
          ems_u32 buf[4];
          dataout_cl[do_idx].errorcode=message.size>=2?message.data[1]:-1;
          dataout_cl[do_idx].do_status=Do_error;
          buf[0]=rtErr_OutDev;
          buf[1]=do_idx;
          buf[2]=0;
          buf[3]=message.size>=2?message.data[1]:-1;
          send_unsolicited(Unsol_RuntimeError, buf, 4);
          if (readout_active) fatal_readout_error();
          special->fatal_expected=1;
          message.size=0;
          message.pid=special->child;
          message.code.req=hndlr_exit;
          msg_write(special->sockets[0], do_idx);
          }
          break;
#endif
        }
      break;
    case hndlc_exit:
      printf("got exit message (%d)\n", message.data[0]);
      if (message.data[0])
        {
        sched_delete_select_task(dataout_cl[do_idx].seltask);
        dataout_cl[do_idx].seltask=0;
        }
      break;
    case hndlc_pid:
      printf("do_tape_handle: pid of handler: %d\n", message.data[0]);
      if (special->child!=message.data[0])
          printf("  sollte aber %d sein\n", special->child);
      if (message.size!=2)
        {
        printf("do_tape_handle: auftape sends wrong message\n");
        }
      else
        {
        if (AUFTAPE_VERSION!=message.data[1])
          {
          printf("do_tape_handle: auftape has wrong version: %d\n",
              message.data[1]);
          }
        }
      break;
    case hndlc_status:
      printf("do_tape_handle(do_idx=%d) status-message\n", do_idx);
      break;
    case hndlc_write:
      {
      struct Cluster* next;
#ifdef CLUSTERCHECK
      if ((int*)dat->active_cluster-clbuffer!=message.data[0])
        {
        printf("programmfehler in do_tape_handle:hndlc_write\n");
        panic();
        }
#endif /* CLUSTERCHECK */
      if (dat->active_cluster->type!=clusterty_events)
        {
        if (dat->active_cluster->type==clusterty_no_more_data)
          {
          printf("do_tape_handle_message(idx=%d): no_more_data\n", do_idx);
          dataout_cl[do_idx].do_status=Do_done;
          dataout_cl[do_idx].procs.reset(do_idx);
          notify_do_done(do_idx);
          }
        else if (dat->active_cluster->type==clusterty_ved_info)
          {
          dataout_cl[do_idx].vedinfo_sent=1;
          }
        }
      do_statist[do_idx].clusters++;
      do_statist[do_idx].words+=dat->active_cluster->size;
      if (message.data[1] /* early end of tape warning */
          && !special->endoftape_warning)
        {
        ems_u32 buf[4];
        printf("do_cluster_tape: end of tape warning received; ignoring all"
        " further event clusters.\n");
        buf[0]=rtErr_OutDev;
        buf[1]=do_idx;
        buf[2]=3;
        send_unsolicited(Unsol_RuntimeError, buf, 3);
        special->endoftape_warning=1;
        }
      next=dat->active_cluster->next;
      dat->active_cluster->costumers[do_idx]=0;
      if (!--dat->active_cluster->numcostumers)
          clusters_recycle(dat->active_cluster);


      if (special->endoftape_warning)
        { /* ignoring all event clusters */
        int found=0;
        while (next && !found)
          {
          while (next && !next->costumers[do_idx]) next=next->next;
          if (next && next->type==clusterty_events)
            {
            next->costumers[do_idx]=0;
            if (!--next->numcostumers) clusters_recycle(next);
            }
          else
            found=1;
          }
        if (!found) next=0;
        }
      else
        {
        while (next && !next->costumers[do_idx]) next=next->next;
        }

      if ((dat->active_cluster=next)!=0)
          do_tape_write(special->sockets[0], dat->active_cluster, special,
            do_idx);
      }
      break;
    case hndlc_linkdata:
      D(D_MEM, printf("Datamodule %s\n", message.data[0]?"linked":"unlinked");)
      special->linked=message.data[0];
      if (special->linked)
        {
        if (dat->active_cluster==0)
          {
          dat->active_cluster=clusters;
          while (dat->active_cluster && !dat->active_cluster->costumers[do_idx])
              dat->active_cluster=dat->active_cluster->next;
          if (dat->active_cluster)
            {
            do_tape_write(special->sockets[0], dat->active_cluster, special,
                do_idx);
            printf("started via link-message\n");
            }
          }
        }
      break;
    case hndlc_filemark:
      {
      ems_u32 buf[4];
      buf[0]=completion_dataout;
      buf[1]=do_idx;
      buf[2]=dataout_completion_filemark;
      buf[3]=message.data[0];
      printf("dataout %d: wrote filemark.\n", buf[1]);
      send_unsolicited(Unsol_ActionCompleted, buf, 4);
      }
      break;
    case hndlc_rewind:
      {
      ems_u32 buf[4];
      buf[0]=completion_dataout;
      buf[1]=do_idx;
      buf[2]=dataout_completion_rewind;
      buf[3]=message.data[0];
      printf("dataout %d: rewound.\n", buf[1]);
      send_unsolicited(Unsol_ActionCompleted, buf, 4);
      }
      break;
    case hndlc_wind:
      {
      ems_u32 buf[4];
      buf[0]=completion_dataout;
      buf[1]=do_idx;
      buf[2]=dataout_completion_wind;
      buf[3]=message.data[0];
      printf("dataout %d: wound.\n", buf[1]);
      send_unsolicited(Unsol_ActionCompleted, buf, 4);
      }
      break;
    case hndlc_seod:
      {
      ems_u32 buf[4];
      buf[0]=completion_dataout;
      buf[1]=do_idx;
      buf[2]=dataout_completion_seod;
      buf[3]=message.data[0];
      printf("dataout %d: seod.\n", buf[1]);
      send_unsolicited(Unsol_ActionCompleted, buf, 4);
      }
      break;
    case hndlc_unload:
      {
      ems_u32 buf[4];
      buf[0]=completion_dataout;
      buf[1]=do_idx;
      buf[2]=dataout_completion_unload;
      buf[3]=message.data[0];
      printf("dataout %d: unloaded.\n", buf[1]);
      send_unsolicited(Unsol_ActionCompleted, buf, 4);
      }
      break;
    case hndlc_control:
      printf("control ok.\n");
      break;
    default:
      printf("do_tape_handle_message: unknown message %d\n", message.code.conf);
      break;
    }
  }
}
/*****************************************************************************/
static void do_tape_handle(int path, enum select_types selected, union callbackdata data)
{
int do_idx=data.i;

T(cluster/do_cluster_tape:do_tape_handle)

if (selected!=select_read)
  {
  if (selected&select_write)
    {
    printf("do_tape_handle(path=%d; do_idx=%d) ready to write\n", path, data.i);
    }
  else
    {
    printf("Exception in do_tape_handle(path=%d; do_idx=%d)\n", path, data.i);
    }
  dataout_cl[do_idx].errorcode=-1;
  dataout_cl[do_idx].do_status=Do_error;
  sched_delete_select_task(dataout_cl[do_idx].seltask);
  dataout_cl[do_idx].seltask=0;
  }
else
  {
  if (msg_read(path, do_idx)<0)
    {
    printf("Error in do_tape_handle(do_idx=%d): msg_read\n", data.i);
    dataout_cl[do_idx].errorcode=-1;
    dataout_cl[do_idx].do_status=Do_error;
    sched_delete_select_task(dataout_cl[do_idx].seltask);
    dataout_cl[do_idx].seltask=0;
    }
  else
    {
    do_tape_handle_message(do_idx);
    }
  }
}
/*****************************************************************************/
static errcode do_cluster_tape_start(int do_idx)
{
int res;
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
T(cluster/do_cluster_tape:do_cluster_tape_start)
printf("do_cluster_tape_start(idx=%d)\n", do_idx);
message.size=1;
message.pid=special->child;
message.code.req=hndlr_linkdata;
message.data[0]=1;
res=msg_write(special->sockets[0], do_idx);

dataout_cl[do_idx].d.c_data.active_cluster=0;
do_statist[do_idx].clusters=0;
do_statist[do_idx].words=0;
do_statist[do_idx].events=0;
do_statist[do_idx].suspensions=0;

dataout_cl[do_idx].suspended=0;
dataout_cl[do_idx].vedinfo_sent=0;
dataout_cl[do_idx].do_status=Do_running;
special->endoftape_warning=0;
special->records=0;
if (res<0)
  {
  printf("do_cluster_tape_start do_idx=%d: msg_write failed", do_idx);
  return Err_System;
  }
else
  return OK;
}
/*****************************************************************************/
static errcode do_cluster_tape_wind(int do_idx, int offs)
{
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
T(cluster/do_cluster_tape::do_cluster_tape_wind)
if (offs<=UNLOAD_WIND)
  {
  message.size=0;
  message.pid=special->child;
  message.code.req=hndlr_unload;
  }
else if (offs<=-MAX_WIND) /* rewind */
  {
  message.size=1;
  message.data[0]=0;
  message.pid=special->child;
  message.code.req=hndlr_rewind;
  }
else if (offs>=MAX_WIND) /* space to end of data */
  {
  message.size=0;
  message.pid=special->child;
  message.code.req=hndlr_seod;
  }
else if (offs==0) /* write filemark */
  {
  message.size=1;
  message.pid=special->child;
  message.code.req=hndlr_filemark;
  message.data[0]=1;
  }
else /* really space */
  {
  message.size=1;
  message.pid=special->child;
  message.code.req=hndlr_wind;
  message.data[0]=offs;
  }
if (msg_write(special->sockets[0], do_idx)<0)
  return Err_System;
else
  return OK;
}
/*****************************************************************************/
static int test_do_idx(int do_idx)
{
if ((unsigned int)do_idx>MAX_DATAOUT) return -1;
if (dataout[do_idx].bufftyp==-1) return -1;
if (dataout[do_idx].addrtyp!=Addr_Tape) return -1;
return 0;
}
/*****************************************************************************/
static int test_do_busy(int do_idx)
{
return 0;
if (xid[do_idx]>lastxid[do_idx])
  {
  printf("test_do_busy: xid=%d; lastxid=%d; code=%d\n", xid[do_idx],
      lastxid[do_idx], message.code.req);
  }
return (xid[do_idx]>lastxid[do_idx]);
}
/*****************************************************************************/
static errcode do_cluster_tape_status(int do_idx)
{
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
int i, xxid;
T(cluster/do_cluster_tape::do_cluster_tape_status)
if (test_do_busy(do_idx)) return plErr_Busy;
if (special->sockets[0]<0) return Err_System;
message.size=0;
message.pid=special->child;
message.code.req=hndlr_status;
if (msg_write(special->sockets[0], do_idx)<0)
  return Err_System;
xxid=message.xid;
if (msg_read(special->sockets[0], do_idx)<0)
  return Err_System;
while (message.xid<xxid)
  {
  do_tape_handle_message(do_idx);
  if (msg_read(special->sockets[0], do_idx)<0)
    return Err_System;
  }
if ((message.xid!=xxid) || (message.code.conf!=hndlc_status))
  {
  printf("cluster/do_cluster_tape::do_cluster_tape_status: got bad answer:\n"
    "  xid=%d (%d expected), conf.-code=%d\n", message.xid,
    xxid, message.code.conf);
  return Err_System;
  }
if (message.data[0]) /* error */
  {
  for (i=0; i<message.size; i++) *outptr++=message.data[i];
  return Err_System;
  }
else
  {
/*
 *   printf("do_cluster_tape_status:\n");
 *   for (i=1; i<message.size; i++) printf("%x ", message.data[i]);
 *   printf("\n");
 */
  /* das erste Word ist counter, wird aber von 'getoutputstatus' erzeugt */
  for (i=2; i<message.size; i++) *outptr++=message.data[i];
  return OK;
  }
}
/*****************************************************************************/
static int do_cluster_tape_control(int do_idx)
{
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
int i, xxid, res;
T(cluster/do_cluster_tape::do_cluster_tape_control)
message.pid=special->child;
message.code.req=hndlr_control;
if (msg_write(special->sockets[0], do_idx)<0)
  return -1;
xxid=message.xid;
if (msg_read(special->sockets[0], do_idx)<0)
  return -1;
while (message.xid<xxid)
  {
  do_tape_handle_message(do_idx);
  if (msg_read(special->sockets[0], do_idx)<0)
    return -1;
  }
if ((message.xid!=xxid) || (message.code.conf!=hndlc_control))
  {
  printf("cluster/do_cluster_tape::do_cluster_tape_control: got bad answer:\n"
    "  xid=%d (%d expected), conf.-code=%d\n", message.xid,
    xxid, message.code.conf);
  res=-1;
  }
else if (message.data[0]) /* error */
  {
  for (i=0; i<message.size; i++) *outptr++=message.data[i];
  res=-1;
  }
else /* ok */
  {
  for (i=1; i<message.size; i++) *outptr++=message.data[i];
  res=0;
  }
return res;
}
/*****************************************************************************/
plerrcode tape_erase(int do_idx, int immediate)
{
printf("tape_erase(do_idx=%d, immediate=%d)\n", do_idx, immediate);
T(cluster/do_cluster_tape::tape_erase)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=2;
message.data[0]=hndlcode_Erase;
message.data[1]=immediate;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_inquiry(int do_idx)
{
plerrcode res;
T(cluster/do_cluster_tape::tape_inquiry)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=1;
message.data[0]=hndlcode_Inquiry;
res=do_cluster_tape_control(do_idx)?plErr_System:plOK;
return res;
}
/*****************************************************************************/
plerrcode tape_load(int do_idx, int action, int immediate)
{
plerrcode res;
printf("tape_load(do_idx=%d, action=%d, imm=%d)\n", do_idx, action, immediate);
T(cluster/do_cluster_tape::tape_load)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=3;
message.data[0]=hndlcode_Load;
message.data[1]=action;
message.data[2]=immediate;
res=do_cluster_tape_control(do_idx);
printf("do_cluster_tape.c::tape_load(%d, %d, %d): res=%d\n", do_idx, action,
    immediate, res);
return res?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_locate(int do_idx, int location)
{
T(cluster/do_cluster_tape::tape_locate)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=2;
message.data[0]=hndlcode_Locate;
message.data[1]=location;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_clearlog(int do_idx)
{
printf("tape_clearlog(do_idx=%d)\n", do_idx);
T(cluster/do_cluster_tape::tape_logselect)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=1;
message.data[0]=hndlcode_LogSelect;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_modeselect(int do_idx, int density)
{
printf("tape_modeselect(do_idx=%d, density=%d)\n", do_idx, density);
T(cluster/do_cluster_tape::tape_modeselect)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=2;
message.data[0]=hndlcode_ModeSelect;
message.data[1]=density;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_prevent_allow_removal(int do_idx, int prevent)
{
plerrcode res;
T(cluster/do_cluster_tape::tape_prevent_allow_removal)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=2;
message.data[0]=hndlcode_Prevent_Allow;
message.data[1]=prevent;
res=do_cluster_tape_control(do_idx);
printf("do_cluster_tape.c::tape_prevent_allow_removal(%d, %d): res=%d\n", do_idx,
    prevent, res);
return res?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_readpos(int do_idx)
{
T(cluster/do_cluster_tape::tape_readpos)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=1;
message.data[0]=hndlcode_ReadPos;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_rewind(int do_idx, int immediate)
{
T(cluster/do_cluster_tape::tape_rewind)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=2;
message.data[0]=hndlcode_Rewind;
message.data[1]=immediate;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_space(int do_idx, int code, int count)
{
T(cluster/do_cluster_tape::tape_space)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=3;
message.data[0]=hndlcode_Space;
message.data[1]=code;
message.data[2]=count;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_modesense(int do_idx)
{
T(cluster/do_cluster_tape::tape_modesense)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=1;
message.data[0]=hndlcode_ModeSense;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_write(int do_idx, int num, ems_u32* data)
{
int i;
T(cluster/do_cluster_tape::tape_write)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
if (num>50)
  {
  printf("Fuer tape_write sind nur 50 worte erlaubt.");
  return plErr_Other;
  }
message.size=num+2;
message.data[0]=hndlcode_Write;
message.data[1]=num;
for (i=0; i<50; i++) message.data[i+2]=data[i];
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_writefilemarks(int do_idx, int count, int immediate, int force)
{
T(cluster/do_cluster_tape::tape_writefilemarks)
if (test_do_idx(do_idx)) return plErr_ArgRange;
if (test_do_busy(do_idx)) return plErr_Busy;
message.size=4;
message.data[0]=hndlcode_WriteFileMarks;
message.data[1]=count;
message.data[2]=immediate;
message.data[3]=force;
return do_cluster_tape_control(do_idx)?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode tape_debug(int do_idx, int debug)
{
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
T(cluster/do_cluster_tape::tape_debug)

if (test_do_idx(do_idx)) return plErr_ArgRange;
message.size=1;
message.pid=special->child;
message.code.req=hndlr_debug;
message.data[0]=debug;
if (msg_write(special->sockets[0], do_idx)<0)
  return plErr_System;
else
  return plOK;
}
/*****************************************************************************/
static void do_cluster_tape_freeze(int do_idx)
{
T(cluster/do_cluster_tape:do_cluster_tape_freeze)
D(D_TRIG, printf("do_cluster_tape_freeze(do_idx=%d)\n", do_idx);)
if (dataout_cl[do_idx].seltask)
  {
  sched_delete_select_task(dataout_cl[do_idx].seltask);
  dataout_cl[do_idx].seltask=0;
  }
}
/*****************************************************************************/
static errcode
do_cluster_tape_reset(int do_idx)
{
    struct do_special_tape* special=
            (struct do_special_tape*)dataout_cl[do_idx].s;

    T(cluster/do_cluster_tape:do_cluster_tape_reset)

    if (special->records) {
        if (dataout_cl[do_idx].do_status==Do_error) {
            printf("  do_cluster_tape_reset(do=%d): status==Do_error (no filemark)\n",
                do_idx);
        } else { 
            printf("  do_cluster_tape_reset(do=%d); write filemark\n", do_idx);
            message.size=1;
            message.pid=special->child;
            message.code.req=hndlr_filemark;
            message.data[0]=1; /* num */
            msg_write(special->sockets[0], do_idx);
        }
    }

    if (special->linked) {
        message.size=1;
        message.pid=special->child;
        message.code.req=hndlr_linkdata;
        message.data[0]=0;
        msg_write(special->sockets[0], do_idx);
    }

    dataout_cl[do_idx].errorcode=0;
    if (dataout_cl[do_idx].do_status!=Do_done)
        dataout_cl[do_idx].do_status=Do_neverstarted;
    return OK;
}
/*****************************************************************************/
static void do_cluster_tape_cleanup(int do_idx)
{
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
/* callbackdata data; */
int status, res;

T(cluster/do_cluster_tape:do_cluster_tape_cleanup)
if (dataout_cl[do_idx].seltask)
  {
  sched_delete_select_task(dataout_cl[do_idx].seltask);
  dataout_cl[do_idx].seltask=0;
  }
if (special->child) /* the child may be dead already */
  {
  special->fatal_expected=1;
  printf("hier war frueher 'install_childhandler(0, special->child, data)'\n");
  /* install_childhandler(0, special->child, data); */

  if (special->sockets[0]>=0)
    {
    message.size=0;
    message.pid=special->child;
    message.code.req=hndlr_exit;
    msg_write(special->sockets[0], do_idx);
    close(special->sockets[0]);
    close(special->sockets[1]);
    }
  else /* it is nearly impossible */
    {
    printf("do_cluster_tape_cleanup(do_idx=%d): child is alive"
        ", but no socket available\n", do_idx);
    kill(special->child, SIGKILL);
    }

  printf("waiting for process %d to die\n", special->child);
  while (((res=waitpid(special->child, &status, 0))<0) && (errno==EINTR));
  if (res<0) printf("tape_cleanup; wait for pid %d: %s\n", special->child,
      strerror(errno));
  printf("it died.\n");
  special->child=0;
  }
    free(special);
    dataout_cl[do_idx].s=0;
}
/*****************************************************************************/
static void do_cluster_tape_advertise(int do_idx, struct Cluster* cl)
{
    struct do_special_tape* special=
            (struct do_special_tape*)dataout_cl[do_idx].s;

T(cluster/do_cluster_tape:do_cluster_tape_advertise)

if (cl->type==clusterty_no_more_data)
  printf("do_cluster_tape_advertise: no_more_data received\n");

if (cl->predefined_costumers && !cl->costumers[do_idx]) return;

if (dataout_cl[do_idx].doswitch==Do_disabled)
  {
  if (cl->type==clusterty_no_more_data)
    {
    if (dataout_cl[do_idx].d.c_data.active_cluster==0)
      {
      dataout_cl[do_idx].do_status=Do_done;
      dataout_cl[do_idx].procs.reset(do_idx);
      notify_do_done(do_idx);
      return;
      }
    }
  else
    return;
  }

if (dataout_cl[do_idx].do_status!=Do_running &&
    cl->type!=clusterty_no_more_data)
  {
  printf("do_cluster_tape_advertise(do=%d): do_status=", do_idx);
  switch (dataout_cl[do_idx].do_status)
    {
    case Do_running:      printf("Do_running\n"); break;
    case Do_neverstarted: printf("Do_neverstarted\n"); break;
    case Do_done:         printf("Do_done\n"); break;
    case Do_error:        printf("Do_error\n"); break;
    case Do_flushing:     printf("Do_flushing\n"); break;
    default: printf("(unknown) %d\n", dataout_cl[do_idx].do_status); break;
    }
/*
 *   if (dataout_cl[do_idx].do_status==Do_error)
 *     {
 *     printf("errorcode=%d\n", dataout_cl[do_idx].errorcode);
 *     }
 *   if (((dataout_cl[do_idx].do_status!=Do_error) || 
 *       (dataout_cl[do_idx].errorcode!=ENOSPC)) &&
 *       (cl->type!=clusterty_text))
 *     {
 *     *(int*)0=0;
 *     return;
 *     }
 *   printf("try to write cluster anyway\n");
 */
  }

if (special->endoftape_warning && (cl->type==clusterty_events))
  return;

cl->costumers[do_idx]=1;
cl->numcostumers++;

if (cl->type==clusterty_no_more_data)
    printf("do_cluster_tape_advertise: will append no_more_data\n");

if (special->linked)
  {
  if (dataout_cl[do_idx].d.c_data.active_cluster==0)
    {
    dataout_cl[do_idx].d.c_data.active_cluster=cl;
    do_tape_write(special->sockets[0], cl, special,
        do_idx);
    if (cl->type==clusterty_no_more_data)
        printf("do_cluster_tape_advertise: no_more_data written\n");
    }
  else
    {
    if (cl->type==clusterty_no_more_data)
        printf("do_cluster_tape_advertise: no_more_data appended\n");
    }
  }
else
  printf("do_cluster_tape_advertise(do=%d): not linked\n", do_idx);
}
/*****************************************************************************/

static void tapechildhandler(pid_t pid, int status, union callbackdata data)
{
int do_idx=data.i, typ=0, code=0;
struct do_special_tape* special=(struct do_special_tape*)dataout_cl[do_idx].s;
int fatal=0;
int old=pid!=special->child;
T(cluster/do_cluster_tape:tapechildhandler)
if (WIFEXITED(status))
  {
  int exitstatus=WEXITSTATUS(status);
  printf("%stapehandler for dataout %d terminated (pid=%d); exitcode %d\n",
    old?"old ":"", do_idx, pid, exitstatus);
  fatal=1; typ=1; code=exitstatus;
  }
if (WIFSIGNALED(status))
  {
  int signal=WTERMSIG(status);
  printf("%stapehandler for dataout %d terminated (pid=%d); signal %d\n",
    old?"old ":"", do_idx, pid, signal);
  fatal=1; typ=2; code=signal;
  }
if (WIFSTOPPED(status))
  {
  int signal=WSTOPSIG(status);
  printf("%stapehandler for dataout %d suspended (pid=%d); signal %d\n",
    old?"old ":"", do_idx, pid, signal);
  typ=3;
  }
#ifdef __osf__
if (WIFCONTINUED(status))
  {
  printf("%stapehandler for dataout %d reactivated (pid=%d)\n",
    old?"old ":"", do_idx, pid);
  typ=4;
  }
#endif
if (fatal)
  {
  install_childhandler(0, pid, data, "tapechildhandler/fatal");
  if (!old)
    {
    close(special->sockets[0]);
    close(special->sockets[1]);
    special->sockets[0]=-1;
    special->sockets[1]=-1;
    }
  }
if (old) return;

if (fatal && !special->fatal_expected)
  {
  ems_u32 buf[5];
  special->child=0;
  dataout_cl[do_idx].errorcode=-1;
  dataout_cl[do_idx].do_status=Do_error;
  if (dataout_cl[do_idx].seltask)
    {
    sched_delete_select_task(dataout_cl[do_idx].seltask);
    dataout_cl[do_idx].seltask=0;
    }
  buf[0]=rtErr_OutDev;
  buf[1]=do_idx;
  buf[2]=2;
  buf[3]=typ;
  buf[4]=code;
  send_unsolicited(Unsol_RuntimeError, buf, 5);
  if (readout_active) fatal_readout_error();
  }
}

/*****************************************************************************/

static struct do_procs tape_procs={
  do_cluster_tape_start,
  do_cluster_tape_reset,
  do_cluster_tape_freeze,
  do_cluster_tape_advertise,
  /*patherror*/ do_NotImp_void_ii,
  do_cluster_tape_cleanup,
  do_cluster_tape_wind,
  do_cluster_tape_status,
  /*filename*/ 0,
  };

/*****************************************************************************/
errcode do_cluster_tape_init(int do_idx)
{
struct do_special_tape* special;
union callbackdata data;
int res;
char name[100];
errcode err=OK;

T(cluster/do_cluster_tape:do_cluster_tape_init)
data.i=do_idx;

if (!cluster_using_shm())
  {
  printf("do_cluster_tape_init: must start server with \"-o :alloc=shm\".\n");
  fflush(stdout);
  return Err_Other;
  }

    special=calloc(1, sizeof(struct do_special_tape));
    if (!special) {
        complain("cluster_tape_init: %s", strerror(errno));
        return Err_NoMem;
    }
    dataout_cl[do_idx].s=special;
    special->sockets[0]=-1;
    special->sockets[1]=-1;

dataout_cl[do_idx].procs=tape_procs;
dataout_cl[do_idx].errorcode=0;
dataout_cl[do_idx].do_status=Do_neverstarted;
dataout_cl[do_idx].doswitch=
    readout_active==Invoc_notactive?Do_enabled:Do_disabled;

special->linked=0;
special->records=0;
special->endoftape_warning=0;
xid[do_idx]=0;
lastxid[do_idx]=0;

if (socketpair(AF_UNIX, SOCK_STREAM, 0, special->sockets)<0)
  {
  *outptr++=errno;
  printf("do_cluster_tape_init: socketpair: %s\n", strerror(errno));
  err=Err_System;
  goto error;
  }
if ((special->child=fork())<0)
  {
  *outptr++=errno;
  printf("do_cluster_tape_init: fork: %s\n", strerror(errno));
  err=Err_System;
  goto error;
  }
if (special->child==0) /* wir sind das Kind */
  {
  char* args[6];
  char vs[256];
  char dens[256];
  unsigned int i;
  if (special->sockets[1]!=3)
    {
    dup2(special->sockets[1], 3);
    close(special->sockets[1]);
    }
  /*for (i=4; i<getdtablesize(); i++) close(i);*/
  printf("arg_num=%d\n", dataout[do_idx].address_arg_num);
  for (i=0; i<dataout[do_idx].address_arg_num; i++)
    {
    printf("[%d] %d\n", i, dataout[do_idx].address_args[i]);
    }
  snprintf(vs, 256, "%d", AUFTAPE_VERSION);
  if (dataout[do_idx].address_arg_num)
    snprintf(dens, 256, "%d", dataout[do_idx].address_args[0]);
  else
    snprintf(dens, 256, "%d", 0);
  args[0]=TAPEHANDLER;
  args[1]=(char*)clusterpool_name();
  args[2]=dataout[do_idx].addr.filename;
  args[3]=dens;
  args[4]=vs;
  args[5]=0;
  execvp(TAPEHANDLER, args);
  /* wenn wir hier sind, sind wir falsch */
  printf("do_cluster_tape_init: exec %s schlug fehl: %s\n", TAPEHANDLER,
      strerror(errno));
  message.size=2;
  message.pid=getpid();
  message.code.conf=hndlc_error;
  message.data[0]=hndle_exec;
  message.data[1]=errno;
  if ((res=write(4, (char*)&message, 6*sizeof(int)))!=6*sizeof(int))
    {
    printf("do_cluster_tape_init: write exec error-message: %s; res=%d\n",
      strerror(errno), res);
    }
  sleep(10);
  close(3);
  exit(0);
  }
/* hier sollten wir, wenn alles gut gegangen ist, ein Kind haben */
/* warten wir auf seinen ersten Schrei */
special->fatal_expected=0;
install_childhandler(tapechildhandler, special->child, data,
    "do_cluster_tape_init");
snprintf(name, 100, "do_%02d_handle_tape", do_idx);
dataout_cl[do_idx].seltask=sched_insert_select_task(do_tape_handle, data,
    name, special->sockets[0],
    select_read|select_except
#ifdef SELECT_STATIST
        , 1
#endif
    );
if (dataout_cl[do_idx].seltask==0)
  {
  printf("do_cluster_tape_init: can't create task. "
      "(Achtung! Das Kind lebt noch)\n");
  err=Err_System;
  goto error;
  }

    return OK;

error:
    close(special->sockets[0]);
    close(special->sockets[1]);
    free(special);
    dataout_cl[do_idx].s=0;

    return err;
}
/*****************************************************************************/
/*****************************************************************************/
