/******************************************************************************
*                                                                             *
* gsitrigmod.c for camac                                                      *
*                                                                             *
* OS9/UNIX                                                                    *
*                                                                             *
* created 05.12.94                                                            *
* last changed 06.12.94                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: gsitrigmod.c,v 1.3 2011/04/06 20:30:35 wuestner Exp $";

#if defined(unix) || defined(__unix__)
#include <sys/time.h>
#endif

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>

#include "../../../lowlevel/camac/camac.h"
#include "gsitrigger.h"
#include "gsitrigmod.h"

static camadr_t stat_r, stat_w;
static camadr_t ctrl_w, fca_w, cvt_w;
static int nummer;

RCS_REGISTER(cvsid, "trigger/camac/gsi_test")

/*****************************************************************************/

plerrcode trigmod_init(int slot, int master, int ctime, int fcatime)
{
int status;

T(trigmod_init)
if ((unsigned int)slot>30) return(plErr_BadHWAddr);
if (get_mod_type(slot)!=GSI_TRIGGER) return(plErr_BadModTyp);

#ifdef NANO
p[3]/=100;
p[4]/=100;
#endif

/* conv. time > fast clear window ? */
if (!ctime || !fcatime || ((ctime | fcatime)&0xffff0000) || (ctime<=fcatime))
    return(plErr_ArgRange);

stat_w=CAMACaddr(slot, STATUS, 16);
stat_r=CAMACaddr(slot, STATUS,  0);
ctrl_w=CAMACaddr(slot, CONTROL,16);
fca_w=CAMACaddr(slot,  FCATIME,16);
cvt_w=CAMACaddr(slot,  CTIME,  16);

CAMACwrite(ctrl_w, CLEAR);
CAMACwrite(ctrl_w, (master?MASTER:SLAVE));
CAMACwrite(ctrl_w, BUS_ENABLE);
CAMACwrite(cvt_w, 0x10000-ctime);
CAMACwrite(fca_w, 0x10000-fcatime);

CAMACwrite(ctrl_w,CLEAR);
CAMACwrite(ctrl_w,GO);

return(plOK);
}

/*****************************************************************************/

plerrcode trigmod_done()
{
T(trigmod_done)
CAMACwrite(ctrl_w, HALT);
CAMACwrite(ctrl_w, BUS_DISABLE);
return(plOK);
}

/*****************************************************************************/

int trigmod_get()
{
register int status, trigger;

T(trigmod_get)

if (((short)(status=CAMACread(stat_r)))>=0) return(0); /*EON nicht gesetzt*/

status=CAMACread(stat_r);

if ((status & EON)==0)
  {
  printf("Trigger: EON==0\n");
  return(0);
  }

trigger=status & 0xf;
if (trigger==0)
  {
  printf("Trigger: trigger==0\n");
  return(0);
  }

if (status & (REJECT | MISMATCH))
  {
  if (status & REJECT)
    {
    printf("Trigger: REJECT\n");
    }
  if (status & MISMATCH)
    {
    printf("Trigger: MISMATCH\n");
    }
  return(0);
  }
return(trigger);
}

/*****************************************************************************/

void trigmod_reset()
{
T(trigmod_reset)
CAMACwrite(stat_w,EV_IRQ_CLEAR);
CAMACwrite(stat_w,DT_CLEAR);
}

/*****************************************************************************/

plerrcode trigmod_soft_init(int slot, int id, int ctime, int fcatime)
{
int status;

T(trigmod_soft_init)
if ((unsigned int)slot>30) return(plErr_BadHWAddr);
if (get_mod_type(slot)!=GSI_TRIGGER) return(plErr_BadModTyp);

if ((id<1) || (id>15)) return(plErr_ArgRange);
nummer=id;
#ifdef NANO
p[3]/=100;
p[4]/=100;
#endif

/* conv. time > fast clear window ? */
if (!ctime || !fcatime || ((ctime | fcatime)&0xffff0000) || (ctime<=fcatime))
    return(plErr_ArgRange);

stat_w=CAMACaddr(slot, STATUS, 16);
stat_r=CAMACaddr(slot, STATUS,  0);
ctrl_w=CAMACaddr(slot, CONTROL,16);
fca_w=CAMACaddr(slot,  FCATIME,16);
cvt_w=CAMACaddr(slot,  CTIME,  16);

CAMACwrite(ctrl_w, CLEAR);
CAMACwrite(ctrl_w, MASTER);
CAMACwrite(ctrl_w, BUS_ENABLE);
CAMACwrite(cvt_w, 0x10000-ctime);
CAMACwrite(fca_w, 0x10000-fcatime);

CAMACwrite(ctrl_w,CLEAR);
CAMACwrite(ctrl_w,GO);

return(plOK);
}

/*****************************************************************************/

plerrcode trigmod_soft_done()
{
T(trigmod_soft_done)
CAMACwrite(ctrl_w, HALT);
CAMACwrite(ctrl_w, BUS_DISABLE);
return(plOK);
}

/*****************************************************************************/

int trigmod_soft_get()
{
register int status, trigger;

T(trigmod_soft_get)

CAMACwrite(stat_w, nummer);
#ifdef OSK
tsleep(2);
#endif

if (((short)(status=CAMACread(stat_r)))>=0) return(0); /*EON nicht gesetzt*/

status=CAMACread(stat_r);

if ((status & EON)==0)
  {
  printf("Trigger: EON==0\n");
  return(0);
  }

trigger=status & 0xf;
if (trigger==0)
  {
  printf("Trigger: trigger==0\n");
  return(0);
  }

if (status & (REJECT | MISMATCH))
  {
  if (status & REJECT)
    {
    printf("Trigger: REJECT\n");
    }
  if (status & MISMATCH)
    {
    printf("Trigger: MISMATCH\n");
    }
  return(0);
  }
return(trigger);
}

/*****************************************************************************/

void trigmod_soft_reset()
{
CAMACwrite(stat_w, EV_IRQ_CLEAR);
CAMACwrite(stat_w, DT_CLEAR);
}

/*****************************************************************************/
/*****************************************************************************/
