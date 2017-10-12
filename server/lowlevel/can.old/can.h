/*
 * $ZEL: can.h,v 1.2 2006/05/22 15:25:17 wuestner Exp $
 * ems/server/lowlevel/can/can.h
 * 
 * (re)created: 9.1.2006 pk
 */

#ifndef _can_h_
#define _can_h_

#include <stdio.h>

int can_low_printuse(FILE* outfilepath);
errcode can_low_init(char*);
errcode can_low_done(void);

#endif

/*****************************************************************************/
