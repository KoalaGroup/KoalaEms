/*
 * lowlevel/lvd/sync/lvd_sync.h
 * created 2006-Feb-07 PW
 * $ZEL: lvd_sync.h,v 1.1 2006/02/12 22:59:39 wuestner Exp $
 */

#ifndef _lvd_sync_h_
#define _lvd_sync_h_

#include <sconf.h>
#include <errorcodes.h>

int lvd_sync_low_printuse(FILE* outfilepath);
errcode lvd_sync_low_init(char* arg);
errcode lvd_sync_low_done(void);

#endif
