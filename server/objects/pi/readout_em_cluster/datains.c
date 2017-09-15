/*
 * readout_em_cluster/datains.c
 * created      23.03.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: datains.c,v 1.12 2011/10/17 23:11:41 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <stdio.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <objecttypes.h>
#include <rcs_ids.h>
#include "datains.h"
#include "di_cluster.h"
#include "di_stream.h"
#include "di_opaque.h"
#include "../readout.h"
#include "../../domain/dom_datain.h"

RCS_REGISTER(cvsid, "objects/pi/readout_em_cluster")

Datain_cl datain_cl[MAX_DATAIN];

/*****************************************************************************/

errcode datain_pre_init(int idx, int qlen, ems_u32* q)
{
errcode res;
printf("datain_pre_init(idx=%d, qlen=%d, q=%p)\n", idx, qlen, q);
memset(datain_cl+idx, 0, sizeof(struct Datain_cl));
/*datain_cl[idx].private=0;*/
switch (datain[idx].bufftyp)
  {
  case InOut_Ringbuffer:
#if 0
    switch (datain[idx].addrtyp)
      {
      case Addr_Raw:
        break;
      case Addr_Modul:
        break;
      case Addr_Driver_mapped:
        break;
      case Addr_Driver_mixed:
        break;
      case Addr_Driver_syscall:
        break;
      case Addr_Socket:
        break;
      case Addr_LocalSocket:
        break;
      case Addr_File:
        break;
      case Addr_Tape:
        break;
      case Addr_Null:
        break;
      default: return Err_AddrTypNotImpl;
      }
    break;
#else
    res=Err_BufTypNotImpl;
#endif
#if 0
  case InOut_Stream:
    switch (datain[idx].addrtyp)
      {
      case Addr_Socket:
        res=di_stream_sock_init(idx, qlen, q);
        break;
      case Addr_LocalSocket:
        res=di_stream_lsock_init(idx, qlen, q);
        break;
      case Addr_File:
      case Addr_Tape:
      case Addr_Null:
      case Addr_Raw:
      case Addr_Modul:
      case Addr_Driver_mapped:
      case Addr_Driver_mixed:
      case Addr_Driver_syscall:
      default: res=Err_AddrTypNotImpl;
      }
    break;
#endif
  case InOut_Cluster:
    switch (datain[idx].addrtyp)
      {
      case Addr_Socket:
        res=di_cluster_sock_init(idx, qlen, q);
        break;
#if 0
      case Addr_LocalSocket:
        res=di_cluster_lsock_init(idx, qlen, q);
        break;
#endif
      case Addr_File:
      case Addr_Tape:
      case Addr_Null:
      case Addr_Raw:
      case Addr_Modul:
      case Addr_Driver_mapped:
      case Addr_Driver_mixed:
      case Addr_Driver_syscall:
      default: res=Err_AddrTypNotImpl;
      }
    break;
  case InOut_Opaque:
    switch (datain[idx].addrtyp)
      {
      case Addr_Socket:
      case Addr_V6Socket:
        res=di_opaque_sock_init(idx, qlen, q);
        break;
#if 0
      case Addr_LocalSocket:
        res=di_opaque_lsock_init(idx, qlen, q);
        break;
#endif
      case Addr_File:
      case Addr_Tape:
      case Addr_Null:
      case Addr_Raw:
      case Addr_Modul:
      case Addr_Driver_mapped:
      case Addr_Driver_mixed:
      case Addr_Driver_syscall:
      default: res=Err_AddrTypNotImpl;
      }
    break;
  default: res=Err_BufTypNotImpl;
  }
return res;
}

/*****************************************************************************/

errcode remove_datain(int idx)
{
datain_cl[idx].procs.cleanup(idx);
return OK;
}

/*****************************************************************************/

void freeze_datains()
{
int i;
for (i=0; i<MAX_DATAIN; i++)
  {
  if (datain[i].bufftyp!=-1) datain_cl[i].procs.stop(i);
  }
}

/*****************************************************************************/

void outputbuffer_freed(void)
{
int i;
if (!ved_info_sent)
  {
  /*printf("outputbuffer_freed(): vedinfo noch nicht gesendet\n");*/
  return;
  }
for (i=0; i<MAX_DATAIN; i++)
  {
  if ((datain[i].bufftyp!=-1) 
      && (datain_cl[i].status==Invoc_active)
      && datain_cl[i].suspended)
    {
    /*printf("objects/pi/readout_em_cluster/datains.c:reactivate(%d)\n", i);*/
    datain_cl[i].procs.reactivate(i);
    }
  }
}

/*****************************************************************************/
void datain_request_event(int ved, int evnum)
{
    int i, di=-1, ved_idx=-1;
    /* find datain */
    for (i=0; (di==-1) && (i<MAX_DATAIN); i++) {
        if (datain[i].bufftyp!=-1) {
            int nv;
            for (nv=0; (di==-1) && (nv<datain_cl[i].vedinfos.num_veds); nv++) {
                if (datain_cl[i].vedinfos.info[nv].ved_id==ved) {
                    di=i;
                    ved_idx=nv;
                }
            }
        }
    }
    if (di==-1) {
        printf("datain_request_event(ved=%d, evnum=%d): ved not found\n",
                ved, evnum);
    } else {
        /*printf("datain_request_event(): ved %d in datain %d (ved_idx %d)\n",
                ved, di, ved_idx);*/
        if (datain_cl[di].procs.request_) {
            datain_cl[di].procs.request_(di, ved_idx, evnum);
        } else {
            printf("\007  request=%p\n", datain_cl[di].procs.request_);
            printf("  sende keinen request.\n");
        }
    }
}
/*****************************************************************************/
/*****************************************************************************/
