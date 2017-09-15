static const char* cvsid __attribute__((unused))=
    "$ZEL: wiener.c,v 1.3 2011/04/06 20:30:34 wuestner Exp $";

#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "wiener.h"

extern ems_u32* outptr;

#define WARTE 4
#define WARTE2 50

RCS_REGISTER(cvsid, "procs/profi/wiener")

plerrcode proc_Init_CrateControl(p)
int *p;
{
  int res;
  if(res=activate_client_sap(p[1],255)){
    *outptr++=res;
    return(plErr_HW);
  }
  if(res=activate_client_sap(p[1]+1,255)){
    deactivate_sap(p[1]);
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode proc_Done_CrateControl(p)
int *p;
{
  int res1,res2;
  res1=deactivate_sap(p[1]);
  res2=deactivate_sap(p[1]+1);
  if(res1){
    *outptr++=res1;
    return(plErr_HW);
  }
  if(res2){
    *outptr++=res2;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode proc_Crate_An(p)
int *p;
{
  char arg;
  char buf[256];
  int len,res;
  arg=V_POWER_ON;
  if(res=do_sda(p[1],p[2],1,&arg,1)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE2);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode proc_Crate_Aus(p)
int *p;
{
  char arg;
  char buf[256];
  int len,res;
  arg=V_POWER_OFF;
  if(res=do_sda(p[1],p[2],1,&arg,1)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE2);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode proc_Read_Crate_UI(p)
int *p;
{
  char arg;
  char buf[256];
  int len,res;
  arg=V_GET_UI0+p[3];
  if(res=do_sda(p[1],p[2],1,&arg,1)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  if(len!=4)return(plErr_HW);
  *outptr++=(buf[0]<<8)+buf[1];
  *outptr++=(buf[2]<<8)+buf[3];
  return(plOK); 
}

plerrcode proc_Read_Crate_Temp(p)
int *p;
{
  char arg;
  char buf[256];
  int len,res;
  arg=V_GET_T0+p[3];
  if(res=do_sda(p[1],p[2],1,&arg,1)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  if(len!=1)return(plErr_HW);
  *outptr++=buf[0];
  return(plOK); 
}

plerrcode proc_Read_Crate_Fans(p)
int *p;
{
  char arg;
  char buf[256];
  int len,res,i;
  arg=V_GET_FAN;
  if(res=do_sda(p[1],p[2],1,&arg,1)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  if((len!=5)&&(len!=8))return(plErr_HW);
  for(i=0;i<len;i++)*outptr++=buf[i];
  return(plOK); 
}

plerrcode proc_Set_Crate_Fans(p)
int *p;
{
  char arg[2];
  char buf[256];
  int len,res,i;
  arg[0]=V_SET_FAN;
  arg[1]=(char)p[3];
  if(res=do_sda(p[1],p[2],1,arg,2)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK); 
}

plerrcode proc_Read_Crate_ID(p)
int *p;
{
  char arg;
  char buf[256];
  int len,res;
  arg=H_ID;
  if(res=do_sda(p[1],p[2],1,&arg,1)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  outptr=outnstring(outptr,buf,len);
  return(plOK); 
}

plerrcode proc_Read_Crate_Status(p)
int *p;
{
  char arg;
  char buf[256];
  int len,res;
  arg=V_GET_STATUS;
  if(res=do_sda(p[1],p[2],1,&arg,1)){
    *outptr++=res;
    return(plErr_HW);
  }
  tsleep(WARTE);
  len=256;
  if(res=do_srd(p[1]+1,p[2],2,"",0,buf,&len)){
    *outptr++=res;
    return(plErr_HW);
  }
  if(len!=2)return(plErr_HW);
  *outptr++=(buf[0]<<8)+buf[1];
  return(plOK); 
}

plerrcode proc_Crate_ResetComm(p)
int *p;
{
  char arg[2];
  int res;
  arg[0]=0x55;
  arg[1]=127;
  if(res=do_sda(p[1],p[2],0,arg,2)){
    *outptr++=res;
    return(plErr_HW);
  }
  return(plOK);
}

plerrcode test_proc_Init_CrateControl(p)
int *p;
{
  if(p[0]!=1)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Done_CrateControl(p)
int *p;
{
  if(p[0]!=1)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Crate_An(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Crate_Aus(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Read_Crate_UI(p)
int *p;
{
  if(p[0]!=3)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Read_Crate_Temp(p)
int *p;
{
  if(p[0]!=3)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Read_Crate_Fans(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Set_Crate_Fans(p)
int *p;
{
  if(p[0]!=3)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Read_Crate_ID(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Read_Crate_Status(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  return(plOK);
}
plerrcode test_proc_Crate_ResetComm(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  return(plOK);
}

char name_proc_Init_CrateControl[]="Init_CrateControl";
char name_proc_Done_CrateControl[]="Done_CrateControl";
char name_proc_Crate_An[]="Crate_An";
char name_proc_Crate_Aus[]="Crate_Aus";
char name_proc_Read_Crate_UI[]="Read_Crate_UI";
char name_proc_Read_Crate_Temp[]="Read_Crate_Temp";
char name_proc_Read_Crate_Fans[]="Read_Crate_Fans";
char name_proc_Set_Crate_Fans[]="Set_Crate_Fans";
char name_proc_Read_Crate_ID[]="Read_Crate_ID";
char name_proc_Read_Crate_Status[]="Read_Crate_Status";
char name_proc_Crate_ResetComm[]="Crate_ResetComm";

int ver_proc_Init_CrateControl=1;
int ver_proc_Done_CrateControl=1;
int ver_proc_Crate_An=1;
int ver_proc_Crate_Aus=1;
int ver_proc_Read_Crate_UI=1;
int ver_proc_Read_Crate_Temp=1;
int ver_proc_Read_Crate_Fans=1;
int ver_proc_Set_Crate_Fans=1;
int ver_proc_Read_Crate_ID=1;
int ver_proc_Read_Crate_Status=1;
int ver_proc_Crate_ResetComm=1;
