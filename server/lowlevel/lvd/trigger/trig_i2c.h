/*
 * lowlevel/lvd/trigger/trig_i2c.h
 * created 2010-Feb-21 PW
 *
 * "$ZEL: trig_i2c.h,v 1.1 2010/04/20 17:06:13 wuestner Exp $"
 */

#ifndef _trigger_trig_i2c_h_
#define _trigger_trig_i2c_h_

struct i2c_addr {
    struct lvd_dev *dev;
    ems_u32 csr;
    ems_u32 data;
    ems_u32 wr;
    ems_u32 rd;
};

plerrcode trig_i2c_write(struct i2c_addr*, int region, int addr, int num,
        ems_u32 *data);
plerrcode trig_i2c_read(struct i2c_addr*, int region, int addr, int num,
        ems_u32 *ptr);
void trig_i2c_decode_dia(ems_u32 *d);
void trig_i2c_decode_mem(ems_u32 *d);

#endif
