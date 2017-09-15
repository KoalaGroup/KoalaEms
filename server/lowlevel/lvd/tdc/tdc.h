/*
 * lowlevel/lvd/tdc/tdc.h
 * $ZEL: tdc.h,v 1.3 2013/01/17 22:44:55 wuestner Exp $
 * created 2006-May-08 PW
 */

#ifndef _lvd_tdc_tdc_h_
#define _lvd_tdc_tdc_h_

#include "../lvdbus.h"

plerrcode lvd_tdc_init(struct lvd_dev* dev, int addr, int gpx, int edges,
        int daqmode);
plerrcode lvd_tdc_edge(struct lvd_dev* dev, int addr, int channel,
        int edges, int immediate);
plerrcode hsdiv_search(struct lvd_dev* dev, int addr, int chip,
        int* min, int* max, int version);
plerrcode lvd_tdc_dac(struct lvd_dev* dev, int addr, int connector,
        int dac, int version);
plerrcode lvd_tdc_window(struct lvd_dev* dev, int addr, int latency,
    int width, int raw);
plerrcode lvd_tdc_getwindow(struct lvd_dev* dev, int addr, ems_u32* latency,
    ems_u32* width, int raw);
plerrcode lvd_tdc_mask(struct lvd_dev*, int addr, u_int32_t* mask);
plerrcode lvd_tdc_get_mask(struct lvd_dev*, int addr, u_int32_t* mask);

#endif
