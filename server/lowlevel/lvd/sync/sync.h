/*
 * lowlevel/lvd/sync/sync.h
 * $ZEL: sync.h,v 1.11 2009/04/01 16:01:01 wuestner Exp $
 * created 2006-Feb-07 PW
 */

#ifndef _lvd_sync_h_
#define _lvd_sync_h_

#include "../lvdbus.h"

plerrcode lvd_syncmaster_init(struct lvd_dev* dev, int addr, int daqmode,
        int trig_mask, int sum_mask);
plerrcode lvd_syncoutput_init(struct lvd_dev* dev, int addr, int daqmode, int flags);

plerrcode lvd_syncmaster_width(struct lvd_dev*, int, int sel, int val, int save);

plerrcode lvd_syncmaster_pause(struct lvd_dev *dev, int addr);
plerrcode lvd_syncmaster_resume(struct lvd_dev *dev, int addr);
plerrcode lvd_syncmaster_start(struct lvd_dev *dev, int addr);
plerrcode lvd_syncmaster_stop(struct lvd_dev *dev, int addr);
plerrcode lvd_syncmaster_evc(struct lvd_dev*, int, ems_u32*);

plerrcode lvd_syncoutput_sram_fill(struct lvd_dev* dev, int idx, unsigned int start,
        int num, int val);
plerrcode lvd_syncoutput_sram_dump(struct lvd_dev* dev, int idx, unsigned int start,
        unsigned int num);
plerrcode lvd_syncoutput_sram_set(struct lvd_dev* dev, int idx, unsigned int addr,
        int val);
plerrcode lvd_syncoutput_sram_get(struct lvd_dev* dev, int idx, unsigned int addr,
        ems_u32* val);
plerrcode lvd_syncoutput_sram_setbits(struct lvd_dev* dev, int idx, int addr,
        int bits);
plerrcode lvd_syncoutput_sram_resbits(struct lvd_dev* dev, int idx, int addr,
        int bits);
plerrcode lvd_syncoutput_sram_bitmasks(struct lvd_dev*, int idx, ems_u32* masks);
plerrcode lvd_syncoutput_sram_check(struct lvd_dev* dev, int idx);
plerrcode lvd_syncoutput_evc(struct lvd_dev*, int, ems_u32*);

#endif
