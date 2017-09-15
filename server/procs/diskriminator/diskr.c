static const char* cvsid __attribute__((unused))=
    "$ZEL: diskr.c,v 1.4 2011/04/06 20:30:31 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>

extern ems_u32* outptr;
extern int *memberlist;

#define D_CRATE(adr) ((adr)>>16)
#define D_SLOT(adr) ((adr)&0xffff)

#define ADR_I 0x10
#define ADR_T 0x11
#define ADR_MH 0x14
#define ADR_ML 0x15
#define ADR_P 0x16

#define D_ADR(slot,adr) (((slot)<<7)|((adr)<<2))
/*#define D_ADR(slot,adr) (((adr)<<6)|((slot)<<2))*/

RCS_REGISTER(cvsid, "procs/diskriminator")

proc_Diskr_SetU(p)
int *p;
{
  int pos,chan,anz,modul,i;
  pos=p[1];
  chan=p[2];
  anz=p[0]-2;
  modul=memberlist[pos];
  for(i=0;i<anz;i++){
    vicwrite(D_CRATE(modul),0,D_ADR(D_SLOT(modul),chan),p[i+3]);
    if((++chan)>15){
      chan=0;
      modul=memberlist[++pos];
    }
  }
  return(plOK);
}

proc_Diskr_ReadU(p)
int *p;
{
  int pos,chan,anz,modul,i;
  pos=p[1];
  chan=p[2];
  anz=p[3];
  modul=memberlist[pos];
  for(i=0;i<anz;i++){
    int val;
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),chan),&val);
    *outptr++=val;
    if((++chan)>15){
      chan=0;
      modul=memberlist[++pos];
    }
  }
  return(plOK);
}

proc_Diskr_SetT(p)
int *p;
{
  int pos,anz,i;
  pos=p[1];
  anz=p[0]-1;
  for(i=0;i<anz;i++){
    int modul;
    modul=memberlist[pos+i];
    vicwrite(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_T),p[i+2]);
  }
  return(plOK);
}

proc_Diskr_ReadT(p)
int *p;
{
  int pos,anz,i;
  pos=p[1];
  anz=p[2];
  for(i=0;i<anz;i++){
    int modul,val;
    modul=memberlist[pos+i];
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_T),&val);
    *outptr++=val;
  }
  return(plOK);
}

proc_Diskr_SetI(p)
int *p;
{
  int pos,anz,i;
  pos=p[1];
  anz=p[0]-1;
  for(i=0;i<anz;i++){
    int modul;
    modul=memberlist[pos+i];
    vicwrite(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_I),p[i+2]);
  }
  return(plOK);
}

proc_Diskr_ReadI(p)
int *p;
{
  int pos,anz,i;
  pos=p[1];
  anz=p[2];
  for(i=0;i<anz;i++){
    int modul,val;
    modul=memberlist[pos+i];
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_I),&val);
    *outptr++=val;
  }
  return(plOK);
}

proc_Diskr_Enable(p)
int *p;
{
  int pos,chan,anz,modul;
  pos=p[1];
  chan=p[2];
  anz=p[3];
  modul=memberlist[pos];
  while(anz){
    int chans,help,mask;
    chans=(anz<(16-chan)?anz:16-chan);
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_MH),&help);
    mask=help<<8;
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_ML),&help);
    mask|=help&0xff;
    mask&=((1<<chan)-1)|(-1<<(chan+chans));
    vicwrite(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_MH),mask>>8);
    vicwrite(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_ML),mask);
    chan=0;
    anz-=chans;
    modul=memberlist[++pos];
  }
  return(plOK);
}

proc_Diskr_Disable(p)
int *p;
{
  int pos,chan,anz,modul;
  pos=p[1];
  chan=p[2];
  anz=p[3];
  modul=memberlist[pos];
  while(anz){
    int chans,help,mask;
    chans=(anz<(16-chan)?anz:16-chan);
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_MH),&help);
    mask=help<<8;
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_ML),&help);
    mask|=help&0xff;
    mask|=(-1<<chan)&((1<<(chan+chans))-1);
    vicwrite(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_MH),mask>>8);
    vicwrite(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_ML),mask);
    chan=0;
    anz-=chans;
    modul=memberlist[++pos];
  }
  return(plOK);
}

proc_Diskr_ReadPattern(p)
int *p;
{
  int pos,anz,i;
  pos=p[1];
  anz=p[2];
  for(i=0;i<anz;i++){
    int modul,val;
    modul=memberlist[pos+i];
    vicread(D_CRATE(modul),0,D_ADR(D_SLOT(modul),ADR_P),&val);
    *outptr++=val;
  }
  return(plOK);
}

plerrcode test_proc_Diskr_SetU(p)
int *p;
{
  if(p[0]<3)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_ReadU(p)
int *p;
{
  if(p[0]!=3)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_SetT(p)
int *p;
{
  if(p[0]<2)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_ReadT(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_SetI(p)
int *p;
{
  if(p[0]<2)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_ReadI(p)
int *p;
{
  if(p[0]<2)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_Enable(p)
int *p;
{
  if(p[0]!=3)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_Disable(p)
int *p;
{
  if(p[0]!=3)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}
plerrcode test_proc_Diskr_ReadPattern(p)
int *p;
{
  if(p[0]!=2)return(plErr_ArgNum);
  if(!memberlist)return(plErr_NoISModulList);
  return(plOK);
}

char name_proc_Diskr_SetU[]="Diskr_SetU";
char name_proc_Diskr_ReadU[]="Diskr_ReadU";
char name_proc_Diskr_SetT[]="Diskr_SetT";
char name_proc_Diskr_ReadT[]="Diskr_ReadT";
char name_proc_Diskr_SetI[]="Diskr_SetI";
char name_proc_Diskr_ReadI[]="Diskr_ReadI";
char name_proc_Diskr_Enable[]="Diskr_Enable";
char name_proc_Diskr_Disable[]="Diskr_Disable";
char name_proc_Diskr_ReadPattern[]="Diskr_ReadPattern";

int ver_proc_Diskr_SetU=1;
int ver_proc_Diskr_ReadU=1;
int ver_proc_Diskr_SetT=1;
int ver_proc_Diskr_ReadT=1;
int ver_proc_Diskr_SetI=1;
int ver_proc_Diskr_ReadI=1;
int ver_proc_Diskr_Enable=1;
int ver_proc_Diskr_Disable=1;
int ver_proc_Diskr_ReadPattern=1;
