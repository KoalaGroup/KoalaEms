/******************************************************************************
*                                                                             *
*  ext_change.c                                                               *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  27.01.94                                                                   *
*                                                                             *
******************************************************************************/

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../lowlevel/chi/chi_map.h"

static int last;
extern ems_u32 eventcnt;

/*****************************************************************************/

void set_outLevel(i)
int i;
{
char lockreg;
char volatile *LOCKREGPTR;

LOCKREGPTR=(char*)REG_BASE+LOCKREG;
lockreg=*LOCKREGPTR;
lockreg&=~(ENLEVEL|LEVEL);
if (i) lockreg|=LEVEL;
*LOCKREGPTR=lockreg;
}

/*****************************************************************************/

int get_extstatus()
{
int res;
char pollreg;
char volatile *POLLREGPTR;

POLLREGPTR=(char*)REG_BASE+POLLREG0;
pollreg=*POLLREGPTR;
res=((pollreg&EXTERN)!=0);
return(res);
}

/*****************************************************************************/

errcode init_trig_ext_change(p)
int *p;
{
if (p[0]!=0) return(Err_ArgNum);

set_outLevel(1);
last=get_extstatus();
eventcnt=0;
return(OK);
}

errcode done_trig_ext_change()
{
set_outLevel(0);
return(OK);
}

int get_trig_ext_change()
{
int res;

res=get_extstatus();
if (res!=last)
  {
  eventcnt++;
  last=res;
  return(res+1);
  }
else
  return(0);
}

void reset_trig_ext_change()
{
sleep(1);
}

char name_trig_ext_change[]="Ext_Change";
int ver_trig_ext_change=1;

/*****************************************************************************/
/*****************************************************************************/

