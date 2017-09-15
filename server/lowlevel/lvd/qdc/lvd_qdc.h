/*
 * lowlevel/lvd/qdc/lvd_qdc.h
 * created 2006-Feb-06 PW
 * $ZEL: lvd_qdc.h,v 1.2 2006/08/04 19:10:17 wuestner Exp $
 */

#ifndef _lvd_qdc_h_
#define _lvd_qdc_h_

#include <sconf.h>
#include <errorcodes.h>

int lvd_qdc_low_printuse(FILE* outfilepath);
errcode lvd_qdc_low_init(char* arg);
errcode lvd_qdc_low_done(void);

#endif
