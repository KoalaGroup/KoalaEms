/*
 * objects/pi/readout.h
 * $ZEL: readout.h,v 1.11 2010/04/20 10:09:04 wuestner Exp $
 */

#ifndef _readout_h_
#define _readout_h_

#include <sconf.h>
#include <objecttypes.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include "../../main/callbacks.h"

extern InvocStatus readout_active;
extern int event_max; /* defined in objects/pi/pi.c */

void fatal_readout_error(void);
errcode pi_readout_init(void);
errcode pi_readout_done(void);
void check_ro_done(int idx);
/* errcode createreadout(int* p, int num); */

void outputbuffer_freed(void);

#ifdef READOUT_CC
extern int inside_readout;
void invalidate_event(void);
#endif

#endif
