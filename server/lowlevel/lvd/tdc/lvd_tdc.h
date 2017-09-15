/*
 * lowlevel/lvd/tdc/lvd_tdc.h
 * created 13.Dec.2003 PW
 * $ZEL: lvd_tdc.h,v 1.2 2007/03/02 22:46:30 wuestner Exp $
 */

#ifndef _lvd_tdc_h_
#define _lvd_tdc_h_

#include <sconf.h>
#include <errorcodes.h>

int lvd_tdc_low_printuse(FILE* outfilepath);
errcode lvd_tdc_low_init(char* arg);
errcode lvd_tdc_low_done(void);

#endif
