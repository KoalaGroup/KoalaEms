/*
 * lowlevel/jtag.h
 * created 12.Aug.2005 PW
 * $ZEL: jtag.h,v 1.1 2005/11/20 22:04:52 wuestner Exp $
 */

#ifndef _jtag_h_
#define _jtag_h_

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>

int jtag_low_printuse(FILE* outfilepath);
errcode jtag_low_init(char* arg);
errcode jtag_low_done(void);

#endif
