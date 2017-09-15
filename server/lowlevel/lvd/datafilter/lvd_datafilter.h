/*
 * lowlevel/lvd/datafilter/lvd_datafilter.h
 * created 2010-Feb-03 PW
 * $ZEL: lvd_datafilter.h,v 1.1 2010/02/05 14:41:56 wuestner Exp $
 */

#ifndef _lvd_datafilter_h_
#define _lvd_datafilter_h_

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>

int lvd_datafilter_low_printuse(FILE* outfilepath);
errcode lvd_datafilter_low_init(char* arg);
errcode lvd_datafilter_low_done(void);

#endif
