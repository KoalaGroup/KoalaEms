/*
 * lowlevel/lvd/lvd_sync_statist.h
 * $ZEL: lvd_sync_statist.h,v 1.4 2009/02/10 21:22:43 wuestner Exp $
 * created 2006-Feb-07 PW
 */

#ifndef _lvd_sync_statist_h_
#define _lvd_sync_statist_h_

#include "lvdbus.h"

void lvd_sync_statist_clear(struct lvd_dev*);
void lvd_sync_statist_dump(struct lvd_dev*);
plerrcode lvd_get_sync_statist(struct lvd_dev*, ems_u32 *out, int *num, int flags, int max);
plerrcode lvd_sync_statist_autosetup(struct lvd_dev* dev);
int lvd_parse_sync_event(struct lvd_dev* dev, ems_u32* event, int size);

#endif
