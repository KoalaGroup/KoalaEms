/*
 * lowlevel/lvd/trig/trigger.h
 * $ZEL: trigger.h,v 1.3 2011/04/15 19:50:36 wuestner Exp $
 * created 2009-Nov-10 PW
 */

#ifndef _lvd_trigger_h_
#define _lvd_trigger_h_

struct lvd_dev;

plerrcode lvd_trig_init(struct lvd_dev*, int addr, int daqmode);

plerrcode lvd_trig_set_ena(struct lvd_dev* dev, int addr, ems_u32 pattern);
plerrcode lvd_trig_get_ena(struct lvd_dev* dev, int addr, ems_u32* pattern);
plerrcode lvd_trig_get_sync(struct lvd_dev* dev, int addr, ems_u32* pattern);
plerrcode lvd_trig_rmid(struct lvd_dev* dev, int addr, int idx, ems_u32* id);

plerrcode lvd_trig_write_mem(struct lvd_dev* dev, int addr,
    int selector, int layer, int sector, int adr, int num, ems_u32* data);
plerrcode lvd_trig_read_mem(struct lvd_dev* dev, int addr,
    int selector, int layer, int sector, int adr, int num, ems_u32* data);
plerrcode lvd_trig_write_pi(struct lvd_dev* dev, int addr,
    int adr, int num, ems_u32* data);
plerrcode lvd_trig_read_pi(struct lvd_dev* dev, int addr,
    int adr, int num, ems_u32* data);

#endif
