/*
 * lowlevel/ipmod/ipmod.h
 * created 2015-Dec-19 PeWue
 * $ZEL: ipmod.h,v 1.3 2016/07/01 16:35:23 wuestner Exp $
 */

#ifndef _ipmod_h_
#define _ipmod_h_

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>

int ipmod_low_printuse(FILE* outfilepath);
errcode ipmod_low_init(char* arg);
errcode ipmod_low_done(void);

#endif
