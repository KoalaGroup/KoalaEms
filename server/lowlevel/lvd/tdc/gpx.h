/*
 * lowlevel/lvd/tdc/gpx.h
 * $ZEL: gpx.h,v 1.6 2013/01/17 22:44:55 wuestner Exp $
 * created 2006-Feb-07 PW
 */

#ifndef _lvd_gpx_h_
#define _lvd_gpx_h_

struct lvd_dev;
struct lvd_acard;

#include "../lvdbus.h"

plerrcode gpx_init_(struct lvd_dev* dev, struct lvd_acard* acard, int chip,
        int edges, int daqmode);
plerrcode gpx_window_(struct lvd_dev* dev, struct lvd_acard* acard,
    int latency, int width, int raw);
plerrcode gpx_getwindow_(struct lvd_dev* dev, struct lvd_acard* acard,
    ems_u32* latency, ems_u32* width, int raw);
plerrcode gpx_dac_(struct lvd_dev* dev, struct lvd_acard* acard, int connector,
    int dac, int version);
int gpx_write_reg(struct lvd_dev* dev, int addr, int gpx, int reg, int val);
int gpx_read_reg(struct lvd_dev* dev, int addr, int gpx, int reg, ems_u32* val);
plerrcode gpx_edge_(struct lvd_dev* dev, struct lvd_acard* acard, int channel,
    int edges, int immediate);
int gpx_hsdiv_search(struct lvd_dev* dev, int addr, int chip, int* hlow,
    int* hhigh);
plerrcode gpx_dcm_shift(struct lvd_dev*, int addr, int dcm, int abs, int val);
plerrcode gpx_mask_(struct lvd_dev*, struct lvd_acard* acard, u_int32_t* mask);
plerrcode gpx_get_mask_(struct lvd_dev*, struct lvd_acard* acard,
    u_int32_t* mask);

#endif
