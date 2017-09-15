/******************************************************************************
*                                                                             *
*  chi_ext_trigger.c                                                          *
*                                                                             *
*  Version: Test external signal input CHI                                    *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  94/01/26  mod: Leege / FZR                                                 *
*                                                                             *
*                                                                             *
******************************************************************************/

#include <ptypes.h>
#include <stdio.h>
#include "../../lowlevel/chi/chi_map.h"
#include <sconf.h> 
#include <debug.h>
#include <errorcodes.h>
#include "../../procs/fastbus/fbacc.h"
#include "../../trigprocs.h"

int count;
static int trigger;
extern ems_u32 eventcnt;

/*****************************************************************************/

 
int no_trigger;  /* test */

#ifdef SCHED_TRIGGER
plerrcode init_trig_chi_ext(int* p, trigprocinfo* info)
#else
plerrcode init_trig_chi_ext(int* p)
#endif
{
T(init_trig_chi_ext)

if (p[0]!=1) return(Err_ArgNum);
trigger=p[1];
printf("init trigger=%d\n",trigger);
eventcnt=0;
INIT_LATCH_EXT();
 
no_trigger=0; 
#ifdef SCHED_TRIGGER
info->proctype=tproc_poll;
#endif
return(OK);
}

/**/
plerrcode done_trig_chi_ext()
{return(OK);}



/**/
int get_trig_chi_ext()
 
{
T(get_trig_chi_ext)
/* printf("get trigger=%d\n",trigger); */

/*FBPULSE();*/ 

if( LATCH_TRIGGER())
  {
  no_trigger=0; 

  eventcnt++;
  if(eventcnt%100==0) printf("event:%d\n",eventcnt);
  return(trigger);
  }

/* printf("no trigger=%d\n",trigger); */

no_trigger++;
if(no_trigger>10000)
{ no_trigger=0;
  RESET_TRIGGER();
  latch_clear();  
/* test test test */ }

return(0);
}

/**/
reset_trig_chi_ext()
{
T(reset_trig_chi_ext)
/*printf("reset trigger=%d\n",trigger);*/

RESET_TRIGGER();        /* entgueltig als reset_trig_chi_ext selbst */
return(0);
}

char name_trig_chi_ext[] = "Chi_ext";
int ver_trig_chi_ext = 1;


/*****************************************************************************/
/*****************************************************************************/


