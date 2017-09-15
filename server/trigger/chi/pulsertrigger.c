/******************************************************************************
*                                                                             *
*  pulsertrigger.c                                                            *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  30.04.93                                                                   *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: pulsertrigger.c,v 1.6 2011/04/06 20:30:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#define  time_out 100
#include <rcs_ids.h>
#include "../../trigprocs.h"

static int trigger;
extern ems_u32 eventcnt;

RCS_REGISTER(cvsid, "trigger/chi")

/*****************************************************************************/

#ifdef SCHED_TRIGGER
errcode init_trig_pulser(int* p, trigprocinfo* info)
#else
errcode init_trig_pulser(int* p)
#endif
{
if (p[0]!=1) return(Err_ArgNum);
trigger=p[1];
printf("trigger=%d\n",trigger);
INIT_LATCH_TRIGGER();
eventcnt=0;
#ifdef SCHED_TRIGGER
info->proctype=tproc_poll;
#endif
return(OK);
}

errcode done_trig_pulser()
{
return(OK);
}

int get_trig_pulser()
{
int i, no_trigger=0;

T(get_trig_pulser)

FBPULSE();

do
  {
  no_trigger++;
  } while (no_trigger<time_out && !LATCH_TRIGGER());

if (no_trigger==time_out)
  {
  RESET_TRIGGER();
  clear_latch();
  printf("last evt=%d timeout in waiting for trigger from PULSER\n", eventcnt);
  return(0);
  }
else
  eventcnt++;
if (eventcnt%1000==0) printf("event:%d\n", eventcnt);
return(trigger);
}

reset_trig_pulser()
{
T(reset_trig_pulser)

RESET_TRIGGER();        /* send clear from cat1810 */
}

char name_trig_pulser[]="Pulser";
int ver_trig_pulser=1;

/*****************************************************************************/
/*****************************************************************************/

