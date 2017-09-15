/*
 * lowlevel/lvd/sis1100/sis1100_lvd_open.h
 *
 * $ZEL: sis1100_lvd_open.h,v 1.3 2009/12/03 00:04:54 wuestner Exp $
 */

#ifndef _sis1100_lvd_open_h_
#define _sis1100_lvd_open_h_

#include "../lvdbus.h"
#include "sis1100_lvd.h"
#include "dev/pci/sis1100_var.h"

int sis1100_lvd_open_dev(const char* pathname, enum sis1100_subdev subdev);

int sis1100_lvd_mmap_init(struct lvd_sis1100_link*);
void sis1100_lvd_mmap_done(struct lvd_sis1100_link*);

#endif
