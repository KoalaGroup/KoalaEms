/*
 * lowlevel/asynch_io/asynch_io.h
 * 
 * $ZEL: asynch_io.h,v 1.1 2007/10/24 21:18:04 wuestner Exp $
 */

#ifndef _asynch_io_h_
#define _asynch_io_h_

#include <stdio.h>
#include <errorcodes.h>

int asynch_io_low_printuse(FILE* outfilepath);
errcode asynch_io_low_init(char* arg);
errcode asynch_io_low_done(void);

#endif
