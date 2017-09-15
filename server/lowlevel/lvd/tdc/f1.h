/*
 * lowlevel/lvd/tdc/f1.h
 * $ZEL: f1.h,v 1.5 2013/01/17 22:44:54 wuestner Exp $
 * created 12.Dec.2003 PW
 */

#ifndef _lvd_f1_h_
#define _lvd_f1_h_

#include "../lvdbus.h"

struct f1_info {
    ems_u16 shadow[8][16]; /* [f1][reg] */
    int edges[8][4];       /* [f1][reg-2] */
    int last_f1;
};

plerrcode f1_init_(struct lvd_dev* dev, struct lvd_acard* acard, int chip,
        int edges, int daqmode);
plerrcode f1_window_(struct lvd_dev* dev, struct lvd_acard* acard,
    int latency, int width, int raw);
plerrcode f1_getwindow_(struct lvd_dev* dev, struct lvd_acard* acard,
    ems_u32* latency, ems_u32* width, int raw);
plerrcode f1_dac_(struct lvd_dev* dev, struct lvd_acard* acard, int channel,
    int dac, int version);
int f1_reg(struct lvd_dev* dev, int addr, int f1, int reg, int val);
int f1_regbits(struct lvd_dev* dev, int addr, int f1, int reg, int val,
    int mask);
int f1_state(struct lvd_dev* dev, int card, int f1);
int f1_clear(struct lvd_dev* dev, int card, int f1, ems_u32 val);
int f1_hsdiv_search(struct lvd_dev* dev, int addr, int f1, int* hl, int* hh);
int f1_hsdiv_search2(struct lvd_dev* dev, int addr, int f1, int* hl, int* hh);
int f1_reg_w(struct lvd_dev* dev, struct lvd_acard* acard, unsigned int f1,
    int unsigned reg, int val);
int f1_reg_r(struct lvd_dev* dev, struct lvd_acard* acard, unsigned int f1,
    int unsigned reg, int* val);
int f1_get_shadow(struct lvd_dev* dev, int addr, int f1, int reg,
    ems_u32** out);
int f1_reset_chips(struct lvd_dev* dev, struct lvd_acard* acard);
plerrcode f1_edge_(struct lvd_dev* dev, struct lvd_acard* acard, int channel,
    int edges, int immediate);
plerrcode f1_mask_(struct lvd_dev*, struct lvd_acard* acard, u_int32_t* mask);
plerrcode f1_get_mask_(struct lvd_dev*, struct lvd_acard* acard,
    u_int32_t* mask);

#endif
