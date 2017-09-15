/******************************************************************************
*                                                                             *
*  immerpuls.c                                                                  *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  19.01.94                                                                   *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: immerpuls.c,v 1.7 2011/04/06 20:30:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../trigprocs.h"

static int trigger;
extern ems_u32 eventcnt;

RCS_REGISTER(cvsid, "trigger/chi")

/*****************************************************************************/

#ifdef SCHED_TRIGGER
errcode init_trig_immerpuls(int* p, trigprocinfo* info)
#else
errcode init_trig_immerpuls(int* p)
#endif
{
T(init_trig_immerpuls)
if (p[0]!=1) return(Err_ArgNum);
trigger=p[1];
eventcnt=0;
#ifdef SCHED_TRIGGER
info->proctype=tproc_poll;
#endif
return(OK);
}

int get_trig_immerpuls()
{
int i;
T(get_trig_pulser)
eventcnt++;
FBPULSE();
return(trigger);
}

reset_trig_immerpuls()
{
T(reset_trig_pulser)
}

done_trig_immerpuls()
{
T(done_trig_immerpuls)
}

char name_trig_immerpuls[]="ImmerPuls";
int ver_trig_immerpuls=1;
int lam_trig_immerpuls=0;

/*****************************************************************************/
/*****************************************************************************/

