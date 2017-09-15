/******************************************************************************
*                                                                             *
*  gsitrigger.c for chi                                                       *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  created before: 06.06.94                                                   *
*  last changed: 18.10.94                                                     *
*                                                                             *
*  PW                                                                         *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: gsitrigger.c,v 1.9 2011/04/06 20:30:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <ssm.h>
#include <errno.h>
#include <rcs_ids.h>
#include "../../../lowlevel/chi_gsi/chi_gsi.h"

#include "gsitrigger.h"

#if (MAX_TRIGGER<16)
#ERROR MAX_TRIGGER too small
#endif

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

extern ems_u32 eventcnt;

static VOLATILE int *stat, *ctrl, *fca, *cvt;
static int nummer;

static int hund;

RCS_REGISTER(cvsid, "trigger/chi/gsi")

/*****************************************************************************/
/*
GSI_Trig
p[1] : base (1, 2, 3)
p[2] : Mastermodule (boolean)
p[3] : conversion time
p[4] : fast clear acceptance window
*/
plerrcode init_trig_gsi(p)
int *p;
{
int status;
char* basis;

T(init_trig_gsi)
if (p[0]!=4) return(plErr_ArgNum);

if ((p[1]<1) || (p[1]>3)) return(plErr_ArgRange);
if (!is_gsi_space(p[1])) return(plErr_HWTestFailed);

#ifdef NANO
p[3]/=100;
p[4]/=100;
#endif

/* conv. time > fast clear window ? */
if ((!p[3]) || (!p[4]) || ((p[3] | p[4])&0xffff0000) || (p[3]<=p[4]))
    return(plErr_ArgRange);

basis=GSI_BASE+0x100*p[1];
stat=(int*)(basis+STATUS);
ctrl=(int*)(basis+CONTROL);
fca=(int*)(basis+FCATIME);
cvt=(int*)(basis+CTIME);

*ctrl=CLEAR;
*ctrl=p[2]?MASTER_TRIG:SLAVE_TRIG;
*ctrl=BUS_ENABLE;
*cvt=0x10000-p[3];
*fca=0x10000-p[4];

#ifdef NANO
p[3]*=100;
p[4]*=100;
#endif

*ctrl=CLEAR;

*ctrl=GO;

eventcnt=0;
return(plOK);
}

/*****************************************************************************/

plerrcode done_trig_gsi()
{
T(done_trig_gsi)
*ctrl=HALT;
*ctrl=BUS_DISABLE;
return(plOK);
}

/*****************************************************************************/

int get_trig_gsi()
{
register int status, event, count, trigger;

T(get_trig_gsi)
status=*stat;
if ((status&EON)==0) return(0);

status=*stat;
if ((status&EON)==0)
  {
  printf("Trigger: erwarteter Event: 0x%08x; EON==0\n", eventcnt);
  return(0);
  }

trigger=status&0xf;
if (trigger==0)
  {
  printf("Trigger: erwarteter Event: 0x%08x; trigger==0\n", eventcnt);
  readout_active= -3;
  return(0);
  }
count=0;
event=(status & 0x00001f00) >> 8;

if ((status & (REJECT | MISMATCH)) || (event!=(eventcnt&0x1f)))
  {
  int buf[3], oldcnt;

  oldcnt=eventcnt;
  if (status & REJECT)
    {
    printf("Trigger: REJECT bei eventcnt %x\n", eventcnt);
    }
  if (status & MISMATCH)
    {
    printf("Trigger: MISMATCH bei eventcnt %x\n", eventcnt);
    }
  if (event!=(eventcnt&0x1f))
    {
    printf(
      "Trigger: erwarteter Event: 0x%08x (...%x); gefundener Event: 0x%x\n",
      eventcnt, eventcnt & 0x1f, event);
    }
  buf[0]=rtErr_Trig;
  buf[1]=oldcnt;
  buf[2]=status;
  send_unsolicited(Unsol_RuntimeError, buf, 3);
  readout_active= -3;
  return(0);
  }
eventcnt++;
return(trigger);
}

/*****************************************************************************/

reset_trig_gsi()
{
T(reset_rig_tgsi)
*stat=EV_IRQ_CLEAR;
*stat=DT_CLEAR;
}

/*****************************************************************************/

char name_trig_gsi[]="GSI_Trig";
int ver_trig_gsi=1;

/*****************************************************************************/

int gethund(start)
int start;
{
int date, time, tick;
short day;
int help;

if (_sysdate(3, &time, &date, &day, &tick)==-1)
  {
  printf("_sysdate schlug fehl\n");
  }
help=(date*86400+time)*100+tick;
/* date*86400 gibt Ueberlauf, macht aber nichts */
return(help-start);
}

/*****************************************************************************/
/*
GSI_soft_Trig
p[1] : base (1, 2, 3)
p[2] : nummer
p[3] : conversion time
p[4] : fast clear acceptance window
*/
plerrcode init_trig_gsi_soft(p)
int *p;
{
int status;
char* basis;

T(init_trig_gsi_soft)
if (p[0]!=4) return(plErr_ArgNum);

if ((p[1]<1) || (p[1]>3)) return(plErr_ArgRange);
if (!is_gsi_space(p[1])) return(plErr_HWTestFailed);

if ((p[2]<1) || (p[2]>15)) return(plErr_ArgRange);
nummer=p[2];

#ifdef NANO
p[3]/=100;
p[4]/=100;
#endif

/* conv. time > fast clear window ? */
if ((!p[3]) || (!p[4]) || ((p[3] | p[4])&0xffff0000) || (p[3]<=p[4]))
    return(plErr_ArgRange);

basis=GSI_BASE+0x100*p[1];
stat=(int*)(basis+STATUS);
ctrl=(int*)(basis+CONTROL);
fca=(int*)(basis+FCATIME);
cvt=(int*)(basis+CTIME);

*ctrl=CLEAR;
*ctrl=MASTER_TRIG;
*ctrl=BUS_ENABLE;
*cvt=0x10000-p[3];
*fca=0x10000-p[4];

#ifdef NANO
p[3]*=100;
p[4]*=100;
#endif

*ctrl=CLEAR;

*ctrl=GO;

eventcnt=0;

hund=gethund(0);
return(plOK);
}

/*****************************************************************************/

plerrcode done_trig_gsi_soft()
{
T(done_trig_gsi_soft)
*ctrl=HALT;
*ctrl=BUS_DISABLE;
return(plOK);
}

/*****************************************************************************/

int get_trig_gsi_soft()
{
register int status, event;
int count, trigger;

if (gethund(hund)<20) return(0);

*stat=nummer;
tsleep(2);

status=*stat;
if ((status&EON)==0) return(0);

status=*stat;
if ((status&EON)==0)
  {
  printf("Trigger: erwarteter Event: 0x%08x; EON==0\n", eventcnt);
  return(0);
  }

trigger=status&0xf;
if (trigger==0)
  {
  printf("Trigger: erwarteter Event: 0x%08x; trigger==0\n", eventcnt);
  readout_active= -3;
  return(0);
  }

count=0;
event=(status & 0x00001f00) >> 8;
while ((event!=(eventcnt&0x1f)) && (count++<100))
  {
  tsleep(2);
  status=*stat;
  event=(status & 0x00001f00) >> 8;
  }
if (count>0)
    printf("Trigger: erwarteter Event: 0x%08x; count2=%d\n", eventcnt, count);

if ((status & (REJECT | MISMATCH)) || (event!=(eventcnt&0x1f)))
  {
  int buf[3], oldcnt;

  oldcnt=eventcnt;
  if (status & REJECT)
    {
    printf("Trigger: REJECT bei eventcnt %x\n", eventcnt);
    }
  if (status & MISMATCH)
    {
    printf("Trigger: MISMATCH bei eventcnt %x\n", eventcnt);
    }
  if (event!=(eventcnt&0x1f))
    {
    printf(
      "Trigger: erwarteter Event: 0x%08x (...%x); gefundener Event: 0x%x\n",
      eventcnt, eventcnt & 0x1f, event);
    }
  buf[0]=rtErr_Trig;
  buf[1]=oldcnt;
  buf[2]=status;
  send_unsolicited(Unsol_RuntimeError, buf, 3);
  readout_active= -3;
  return(0);
  }

eventcnt++;
return(status&0xf);
}

/*****************************************************************************/

reset_trig_gsi_soft()
{
*stat=EV_IRQ_CLEAR;
*stat=DT_CLEAR;
hund=gethund(0);
}

/*****************************************************************************/

char name_trig_gsi_soft[]="GSI_soft_Trig";
int ver_trig_gsi_soft=1;

/*****************************************************************************/
/*****************************************************************************/
