/******************************************************************************
*                                                                             *
*  chi_ext_trigger.c                                                          *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  11.08.94                                                                   *
*                                                                             *
*  mz                                                                         *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: chi_ext_trigger.c,v 1.11 2011/04/06 20:30:36 wuestner Exp $";

#include <stdio.h>
#include "../../lowlevel/cosy11/chi_map.h"
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>

static int trigger;
extern ems_u32 eventcnt;

#ifdef PRINT_EVT
extern ems_u32 *outptr, *next_databuf;
#endif

RCS_REGISTER(cvsid, "trigger/chi")

/*****************************************************************************/

errcode init_trig_chi_ext(p)

int *p;
{
T(init_trig_chi_ext)

if (p[0]!=1) return(Err_ArgNum);
trigger=p[1];
printf("trigger=%d\n",trigger);

INIT_LATCH_TRIGGER();
RESET_TRIGGER();
reset_chi_busy();

eventcnt=0;
return(OK);
}

errcode done_trig_chi_ext()
{
return(OK);
}

int get_trig_chi_ext()
{
static int no_trigger=0;

T(NEU:get_trig_chi_ext)

if(LATCH_TRIGGER())
  {
  no_trigger=0;
/*  if(eventcnt%1000==0) printf("event:%d\n",eventcnt);*/
  eventcnt++;
  return(trigger);
  }
else
  {
  no_trigger++;
  if(no_trigger>500000)
    {
    no_trigger=0;
/*    RESET_TRIGGER();
    clear_latch(); */
    printf("last evt=%d timeout in waiting for trigger\n",eventcnt);
    eventcnt++;return(trigger);   /* trotzdem zum auslesen */
    }
  return(0);
  }

}

reset_trig_chi_ext()
{
T(reset_trig_chi_ext)

RESET_TRIGGER();        /* entgueltig als reset_trig_chi_ext selbst */

#ifdef PRINT_EVT
event_print();
#endif
}

char name_trig_chi_ext[]="Chi_ext";
int ver_trig_chi_ext=1;

/*****************************************************************************/

#ifdef PRINT_EVT
event_print()
{
int *i, k, LEN, ISMAX;

i=next_databuf;
printf("    event=%4d \n",*i); i++;
printf("  trigger=%4d \n",*i); i++;
ISMAX=*i++; printf("# of IS's=%4d \n",ISMAX);
for (k=1; k<=ISMAX; k++)
  {
  printf(" IS= %4d \n", *i); i++;
  LEN=*i++; printf(" L=%4d \n",LEN);
  while (LEN) {decode(*i);i++; LEN--;}
  }
}

decode(word)
int word;
{
int geog, chan, time, range;

geog=(word>>27)&0x1f;
time=( (word<< 2) & 0xfff )>>3;
chan=( (word>>15) & 0xff )>>1;
printf("t1879: geog,chan,time:%3d %3d %4d %4d\n", geog,chan,time,512-time);
}
#endif

/*****************************************************************************/
/*****************************************************************************/
