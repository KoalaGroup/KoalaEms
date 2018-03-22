/*
 * objects/pi/readout.h
 * $ZEL: readout.h,v 1.13 2017/10/09 20:48:15 wuestner Exp $
 */

#ifndef _readout_h_
#define _readout_h_

#include <sconf.h>
#include <objecttypes.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include "../../main/callbacks.h"

extern InvocStatus readout_active;
extern size_t event_max; /* defined in objects/pi/pi.c */
extern size_t wirhaben;

void fatal_readout_error(void);
errcode pi_readout_init(void);
errcode pi_readout_done(void);
void check_ro_done(int idx);
/* errcode createreadout(int* p, int num); */

void outputbuffer_freed(void);

#ifdef READOUT_CC
struct global_evc global_evc;

extern int inside_readout;
void invalidate_event(void);

/* we use a structure here, because we do not know yet whether we need
   more members in it. */
struct global_evc {
    ems_u32 ev_count;
};
extern struct global_evc global_evc; /* defined in pi.c */

#endif

#endif
