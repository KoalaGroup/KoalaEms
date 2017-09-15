/******************************************************************************
*                                                                             *
*  chi_ext_trigger.c                                                          *
*                                                                             *
*  Version: Test external signal input CHI                                    *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  94/05/31  mod: Leege / FZR                                                 *
*                                                                             *
*                                                                             *
******************************************************************************/

/*#include <ptypes.h>*/
#include <stdio.h>
/*#include "../../lowlevel/chi/chi_map.h"*/
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

plerrcode init_trig_chi_ext(p)
/**/
int *p;
{
T(init_trig_chi_ext)

if (p[0]!=1) return(Err_ArgNum);
trigger=p[1];
D(D_TRIG, printf("init trigger=%d\n",trigger);)
eventcnt=0;
INIT_LATCH_EXT();       /* lowlevel */

no_trigger=0;
info->proctype=tproc_poll;
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

if( LATCH_TRIGGER() )   /* lowlevel */
  {
  set_chi_busy();       /* lowlevel */
  no_trigger=0;
/* printf("yes trigger=%d\n",trigger); */
  eventcnt++;
D(D_TRIG, if (eventcnt % 10000 == 0) printf("event:%d\n", eventcnt);)
  return(trigger);
  }

/* printf("no trigger=%d\n",trigger); */

no_trigger++;
if ((no_trigger%10000)==0)
  {
  D(D_TRIG, printf("kein Trigger nach %d Versuchen\n", no_trigger);)
  }
if (no_trigger>1000000)
  {
  no_trigger=0;
  RESET_TRIGGER();      /* lowlevel */
  clear_latch();        /* lowlevel */
  }

return(0);
}

/**/
reset_trig_chi_ext()
{
 T(reset_trig_chi_ext)
 /* printf("reset trigger=%d\n",trigger); */
 /* RESET_TRIGGER(); */       /* lowlevel */
 clear_latch();          /* lowlevel */
 reset_chi_busy();       /* lowlevel */
 return(0);
}

char name_trig_chi_ext[] = "Chi_ext";
int ver_trig_chi_ext = 1;


/*****************************************************************************/
/*****************************************************************************/


