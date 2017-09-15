static const char* cvsid __attribute__((unused))=
    "$ZEL: profi.c,v 1.4 2011/04/06 20:30:34 wuestner Exp $";

#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../lowlevel/profibus/profi.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/profi")

plerrcode proc_GetProfiLAS(p)
int *p;
{
  char buf[128];
  int len,res,i;
  len=p[1];
  if(res=profi_getlas(buf,&len)){
    if(res==-2)return(plErr_NoMem);
    *outptr++=res;
    return(plErr_HW);
  }
  for(i=0;i<len;i++)if(buf[i])*outptr++=i;
  return(plOK);
}

plerrcode proc_GetProfiLiveList(p)
int *p;
{
  char buf[128];
  int len,res,i;
  len=p[1];
  if(res=profi_getlivelist(buf,&len)){
    if(res==-2)return(plErr_NoMem);
    *outptr++=res;
    return(plErr_HW);
  }
  for(i=0;i<len;i++)*outptr++=buf[i];
  return(plOK);
}

plerrcode proc_GetProfiStatus(){
  struct profistatus buf;
  int res,i;
  if(res=profi_getstatus(&buf)){
    *outptr++=res;
    return(plErr_HW);
  }
  *outptr++=buf.station;
  *outptr++=buf.hsa;
  *outptr++=buf.baud_rate;
  *outptr++=buf.min_tsdr;
  *outptr++=buf.max_tsdr;
  *outptr++=buf.tset;
  *outptr++=buf.frame_send_count;
  *outptr++=buf.retry_count;
  return(plOK);
}

plerrcode test_proc_GetProfiLAS(p)
int *p;
{
  if(p[0]!=1)return(plErr_ArgNum);
  if(p[1]>128)return(plErr_ArgRange);
  return(plOK);
}

plerrcode test_proc_GetProfiLiveList(p)
int *p;
{
  return(test_proc_GetProfiLAS(p));
}

plerrcode test_proc_GetProfiStatus(p)
int *p;
{
  if(p[0]!=0)return(plErr_ArgNum);
  return(plOK);
}

char name_proc_GetProfiLAS[]="GetProfiLAS";
char name_proc_GetProfiLiveList[]="GetProfiLiveList";
char name_proc_GetProfiStatus[]="GetProfiStatus";
int ver_proc_GetProfiLAS=1;
int ver_proc_GetProfiLiveList=1;
int ver_proc_GetProfiStatus=1;

plerrcode proc_ActivateSAP(p)
int *p;
{
  int res;
  if(res=activate_client_sap(p[1],p[2])){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode proc_DeactivateSAP(p)
int *p;
{
  int res;
  if(res=deactivate_sap(p[1])){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode proc_SDA(p)
int *p;
{
  char buf[256];
  int len,res;
  extractstring(buf,p+4);
  len=xdrstrclen(p+4);
  if(res=do_sda(p[1],p[2],p[3],buf,len)){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode proc_SRD(p)
int *p;
{
  char buf[256],rbuf[256];
  int len,rlen,res;
  extractstring(buf,p+4);
  len=xdrstrclen(p+4);
  rlen=256;
  if(res=do_srd(p[1],p[2],p[3],buf,len,rbuf,&rlen)){
    *outptr++=res;
    return(plErr_HW);
  }
  outptr=outnstring(outptr,rbuf,rlen);
  return(plOK);
}

plerrcode proc_SDN(p)
int *p;
{
  char buf[256];
  int len,res;
  extractstring(buf,p+4);
  len=xdrstrclen(p+4);
  if(res=do_sdn(p[1],p[2],p[3],buf,len)){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode test_proc_ActivateSAP(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  return(plOK);
}

plerrcode test_proc_DeactivateSAP(p)
int *p;
{
  if(p[0]!=1)return(plErr_ArgNum);
  return(plOK);
}

plerrcode test_proc_SDA(p)
int *p;
{
  if(p[0]<4)return(plErr_ArgNum);
  return(plOK);
}

plerrcode test_proc_SRD(p)
int *p;
{
  if(p[0]<4)return(plErr_ArgNum);
  return(plOK);
}

plerrcode test_proc_SDN(p)
int *p;
{
  if(p[0]<4)return(plErr_ArgNum);
  return(plOK);
}

char name_proc_ActivateSAP[]="ActivateSAP";
char name_proc_DeactivateSAP[]="DeactivateSAP";
char name_proc_SDA[]="SDA";
char name_proc_SRD[]="SRD";
char name_proc_SDN[]="SDN";
int ver_proc_ActivateSAP=1;
int ver_proc_DeactivateSAP=1;
int ver_proc_SDA=1;
int ver_proc_SRD=1;
int ver_proc_SDN=1;

plerrcode proc_ProfiIdent(p)
int *p;
{
  char buf[256];
  int len,res,i;
  char *help;
  len=256;
  if(res=profi_ident(p[1],buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  help=buf+4;
  for(i=0;i<4;i++){
    outptr=outnstring(outptr,help,buf[i]);
    help+=buf[i];
  }
  return(plOK);
}

plerrcode test_proc_ProfiIdent(p)
int *p;
{
  if(p[0]!=1)return(plErr_ArgNum);
  return(plOK);
}
char name_proc_ProfiIdent[]="ProfiIdent";
int ver_proc_ProfiIdent=1;
