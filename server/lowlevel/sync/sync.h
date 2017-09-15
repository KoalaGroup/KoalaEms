/*
 * lowlevel/sync/sync.h
 * created 2007-Jul-02 PW
 * $ZEL: sync.h,v 1.1 2007/07/18 16:40:38 wuestner Exp $
 */

#ifndef _sync_h_
#define _sync_h_

#include <stdio.h>

int sync_low_printuse(FILE* outfilepath);
errcode sync_low_init(char*);
errcode sync_low_done(void);

#endif

/*****************************************************************************/
