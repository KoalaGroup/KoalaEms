/*
 * $ZEL: canbus.h,v 1.1 2007/03/02 22:52:47 wuestner Exp $
 * ems/server/lowlevel/canbus/canbus.h
 * 
 * (re)created: 9.1.2006 pk
 */

#ifndef _canbus_h_
#define _canbus_h_

#include <stdio.h>

int canbus_low_printuse(FILE* outfilepath);
errcode canbus_low_init(char*);
errcode canbus_low_done(void);

#endif

/*****************************************************************************/
