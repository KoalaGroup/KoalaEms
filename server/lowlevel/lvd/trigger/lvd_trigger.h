/*
 * lowlevel/lvd/trig/lvd_trigger.h
 * created 2009-Nov-10 PW
 * $ZEL: lvd_trigger.h,v 1.1 2009/12/03 13:37:41 wuestner Exp $
 */

#ifndef _lvd_trigger_h_
#define _lvd_trigger_h_

#include <sconf.h>
#include <errorcodes.h>

int lvd_trigger_low_printuse(FILE* outfilepath);
errcode lvd_trigger_low_init(char* arg);
errcode lvd_trigger_low_done(void);

#endif
