/*
 * readout_eventfilter/datains.c
 * created 2007-Jun-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL$";

#include <string.h>
#include <errorcodes.h>
#include <sconf.h>
#include <xdrstring.h>
#include <objecttypes.h>
#include "datains.h"
/*#include "di_cluster.h"*/
#include "../readout.h"
#include "../../domain/dom_datain.h"

/*Datain_cl datain_cl[MAX_DATAIN];*/ /* needs a different definition */

/*****************************************************************************/
errcode
datain_pre_init(int idx, int qlen, ems_u32* q)
{
    errcode res;
    printf("datain_pre_init(idx=%d, qlen=%d, q=%p)\n", idx, qlen, q);

    printf("readout_eventfilter/datains.c:getreadoutparams: HIER FEHLT CODE\n");

#if 0
switch (datain[idx].bufftyp)
  {
  case InOut_Ringbuffer:
/*  switch (datain[idx].addrtyp)
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
    break; */
    res=Err_BufTypNotImpl;
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
  case InOut_Cluster:
    switch (datain[idx].addrtyp)
      {
      case Addr_Socket:
        res=di_cluster_sock_init(idx, qlen, q);
        break;
      case Addr_LocalSocket:
        res=di_cluster_lsock_init(idx, qlen, q);
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
  default: res=Err_BufTypNotImpl;
  }

#endif
    return res;
}
/*****************************************************************************/
#if 0
errcode remove_datain(int idx)
{
datain_cl[idx].procs.cleanup(idx);
return OK;
}
#endif
/*****************************************************************************/
#if 0
void freeze_datains()
{
int i;
for (i=0; i<MAX_DATAIN; i++)
  {
  if (datain[i].bufftyp!=-1) datain_cl[i].procs.stop(i);
  }
}
#endif
/*****************************************************************************/
#if 0
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
#endif
/*****************************************************************************/
/*****************************************************************************/
