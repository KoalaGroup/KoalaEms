/*
 * lowlevel/lvd/lvd.h
 * created 10.Dec.2003 PW
 * $ZEL: lvd.h,v 1.3 2005/05/27 19:09:02 wuestner Exp $
 */

#ifndef _lvd_h_
#define _lvd_h_

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>

int lvd_low_printuse(FILE* outfilepath);
errcode lvd_low_init(char* arg);
errcode lvd_low_done(void);

#endif
