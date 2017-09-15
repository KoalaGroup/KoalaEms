/******************************************************************************
*                                                                             *
* fbutil.c                                                                    *
*                                                                             *
* created before: 11.08.94                                                    *
* last changed: 25.04.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbutil.c,v 1.4 2011/04/06 20:30:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/fastbus.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/fastbus")

/*****************************************************************************/

int fb_modul_id(int pa)
{
int res;
int debuglevel;

T(fb_modul_id)
debuglevel=getdebuglevel();
setdebuglevel(0);
res=FRC(pa, 0);
setdebuglevel(debuglevel);
if (err_proc==0)
  return(res>>16);
else
  return(err_code?1:0);
}

/*****************************************************************************/

plerrcode proc_FClear(int* p)
{
FBPD(proc_FClear);
FBCLEAR();
return(plOK);
}

plerrcode test_proc_FClear(p)
int *p;
{
FBPD(test_proc_FClear);
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

char name_proc_FClear[]="FClear";
int ver_proc_FClear=1;

/*****************************************************************************/

plerrcode proc_FPulse(int* p)
{
FBPD(proc_FPulse);
FBPULSE();
return(plOK);
}

plerrcode test_proc_FPulse(int* p)
{
FBPD(test_proc_FPulse);
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

char name_proc_FPulse[]="FPulse";
int ver_proc_FPulse=1;

/*****************************************************************************/

plerrcode proc_FGetChi(int* p)
{
FBPD(proc_FGetChi);
*outptr++=get_ga_chi();
return(plOK);
}

plerrcode test_proc_FGetChi(int* p)
{
FBPD(test_proc_FGetChi);
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

char name_proc_FGetChi[]="FGetChi";
int ver_proc_FGetChi=1;

/*****************************************************************************/
/*
p[0] : N0. of parameters (==1)
p[1] : primary address
return:
  0 if slot empty
  1 if error reading ID
  ID if o.k.
*/
plerrcode proc_FModulID(int* p)
{
FBPD(proc_FModulID);
*outptr++=fb_modul_id(p[1]);
return(plOK);
}

plerrcode test_proc_FModulID(int* p)
{
FBPD(test_proc_FModulID);
if (p[0]!=1) return(plErr_ArgNum);
return(plOK);
}

char name_proc_FModulID[]="FModulID";
int ver_proc_FModulID=1;

/*****************************************************************************/
plerrcode proc_FModulList(int* p)
{
int res, i, j, *help;
int debuglevel;

FBPD(proc_FModulList);
help=outptr;
outptr++;
debuglevel=getdebuglevel();
setdebuglevel(0);
for (i=0, j=0; i<=PA_MAX; i++)
  {
  res=FRC(i, 0);
  if (err_proc==0)
    {
    *outptr++=i;
    *outptr++=res>>16;
    j++;
    }
  }
*help=j;
setdebuglevel(debuglevel);
return(plOK);
}

plerrcode test_proc_FModulList(p)
int *p;
{
FBPD(test_proc_FModulList);
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

char name_proc_FModulList[]="FModulList";
int ver_proc_FModulList=1;

/*****************************************************************************/

plerrcode proc_OutClock(int* p)
{
char lockreg;
char volatile *LOCKREGPTR;

LOCKREGPTR=(char*)REG_BASE+LOCKREG;
lockreg=*LOCKREGPTR;
/*printf("old LockReg=0x%x\n", lockreg);*/
lockreg&=~(ENLEVEL|LEVEL);
if (p[1]==1) lockreg|=ENLEVEL;
/*printf("new LockReg=0x%x\n", lockreg);*/
*LOCKREGPTR=lockreg;
lockreg=*LOCKREGPTR;
/*printf("new LockReg(reread)=0x%x\n", lockreg);*/
return(plOK);
}

plerrcode test_proc_OutClock(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
if ((unsigned int)p[1]>1) return(plErr_ArgRange);
return(plOK);
}

char name_proc_OutClock[]="OutClock";
int ver_proc_OutClock=1;

/*****************************************************************************/

plerrcode proc_OutLevel(int* p)
{
char lockreg;
char volatile *LOCKREGPTR;

LOCKREGPTR=(char*)REG_BASE+LOCKREG;
lockreg=*LOCKREGPTR;
/*printf("old LockReg=0x%x\n", lockreg);*/
lockreg&=~(ENLEVEL|LEVEL);
if (p[1]==1) lockreg|=LEVEL;
/*printf("new LockReg=0x%x\n", lockreg);*/
*LOCKREGPTR=lockreg;
lockreg=*LOCKREGPTR;
/*printf("new LockReg(reread)=0x%x\n", lockreg);*/
return(plOK);
}

plerrcode test_proc_OutLevel(int* p)
{
if (p[0]!=1) return(plErr_ArgNum);
if ((unsigned int)p[1]>1) return(plErr_ArgRange);
return(plOK);
}

char name_proc_OutLevel[]="OutLevel";
int ver_proc_OutLevel=1;

/*****************************************************************************/

plerrcode proc_ExtStatus(int* p)
{
int res;
char pollreg;
char volatile *POLLREGPTR;

POLLREGPTR=(char*)REG_BASE+POLLREG0;
pollreg=*POLLREGPTR;
/*printf("pollreg=0x%x\n", pollreg);*/
res=((pollreg&EXTERN)!=0);
*outptr++=res;
return(plOK);
}

plerrcode test_proc_ExtStatus(int* p)
{
if (p[0]!=0) return(plErr_ArgNum);
return(plOK);
}

char name_proc_ExtStatus[]="ExtStatus";
int ver_proc_ExtStatus=1;

/*****************************************************************************/
/*****************************************************************************/
