/*
 * lowlevel/lvd/qdc/qdc.h
 * $ZEL: qdc.h,v 1.15 2013/01/17 22:44:54 wuestner Exp $
 * created 2006-Feb-07 PW
 */

#ifndef _lvd_qdc_h_
#define _lvd_qdc_h_

struct lvd_dev;

plerrcode lvd_qdc_init(struct lvd_dev*, int addr, int daqmode);

plerrcode lvd_qdc_set_sw_start(struct lvd_dev*, int addr, ems_u32 start);
plerrcode lvd_qdc_get_sw_start(struct lvd_dev*, int addr, ems_u32* start);
plerrcode lvd_qdc_set_sw_length(struct lvd_dev*, int addr, ems_u32 length);
plerrcode lvd_qdc_get_sw_length(struct lvd_dev*, int addr, ems_u32* length);
plerrcode lvd_qdc_set_sw_ilength(struct lvd_dev*, int addr, ems_u32 ilength);
plerrcode lvd_qdc_get_sw_ilength(struct lvd_dev*, int addr, ems_u32* ilength);

plerrcode lvd_qdc_set_iw_start(struct lvd_dev*, int addr, ems_u32 start);
plerrcode lvd_qdc_get_iw_start(struct lvd_dev*, int addr, ems_u32* start);
plerrcode lvd_qdc_set_iw_length(struct lvd_dev*, int addr, ems_u32 length);
plerrcode lvd_qdc_get_iw_length(struct lvd_dev*, int addr, ems_u32* length);

plerrcode lvd_qdc_set_rawcycle(struct lvd_dev*, int addr, ems_u32);
plerrcode lvd_qdc_get_rawcycle(struct lvd_dev*, int addr, ems_u32*);

plerrcode lvd_qdc_set_grp_coinc(struct lvd_dev* dev, int addr, ems_u32);
plerrcode lvd_qdc_get_grp_coinc(struct lvd_dev* dev, int addr, ems_u32*);
plerrcode lvd_qdc_set_coinc(struct lvd_dev* dev, int addr, int group, ems_u32);
plerrcode lvd_qdc_get_coinc(struct lvd_dev* dev, int addr, int group, ems_u32*);

plerrcode lvd_qdc_set_dac(struct lvd_dev*, int addr, int channel, ems_u32 dac);
plerrcode lvd_qdc_get_dac(struct lvd_dev*, int addr, ems_u32* dacs);

plerrcode lvd_qdc_set_testdac(struct lvd_dev*, int addr, ems_u32 dac);
plerrcode lvd_qdc_get_testdac(struct lvd_dev*, int addr, ems_u32* dac);
plerrcode lvd_qdc_test_pulse(struct lvd_dev* dev, int addr);

plerrcode lvd_qdc_set_ped(struct lvd_dev*, int addr, int channel, ems_u32);
plerrcode lvd_qdc_get_ped(struct lvd_dev*, int addr, ems_u32*);

plerrcode lvd_qdc_set_inhibit(struct lvd_dev*, int addr, ems_u32 mask);
plerrcode lvd_qdc_get_inhibit(struct lvd_dev*, int addr, ems_u32* mask);

plerrcode lvd_qdc_set_threshold(struct lvd_dev*, int addr, int channel,
        ems_u32 threshold);
plerrcode lvd_qdc_get_threshold(struct lvd_dev*, int addr,
        ems_u32* thresholds);

plerrcode lvd_qdc_set_thrh(struct lvd_dev* dev, int addr, int channel,
        ems_u32 thrh);
plerrcode lvd_qdc_get_thrh(struct lvd_dev* dev, int addr, ems_u32 *thrh);

plerrcode lvd_qdc_set_raw(struct lvd_dev*, int addr, ems_u32 mask);
plerrcode lvd_qdc_get_raw(struct lvd_dev*, int addr, ems_u32* mask);

plerrcode lvd_qdc_set_anal(struct lvd_dev*, int addr, ems_u32 anal);
plerrcode lvd_qdc_get_anal(struct lvd_dev*, int addr, ems_u32* anal);

plerrcode lvd_qdc_noise(struct lvd_dev*, int addr, ems_u32* out);

plerrcode lvd_qdc_mean(struct lvd_dev*, int addr, ems_u32* out);
plerrcode lvd_qdc_ovr(struct lvd_dev*, int addr, ems_u32* out);
plerrcode lvd_qdc_bline_adjust(struct lvd_dev*, int addr, ems_u32* out);

plerrcode lvd_qdc_set_trglevel(struct lvd_dev* dev, int addr, ems_u32 level);
plerrcode lvd_qdc_get_trglevel(struct lvd_dev* dev, int addr, ems_u32 *level);

plerrcode lvd_qdc_set_sc_rate(struct lvd_dev* dev, int addr, ems_u32 rate);
plerrcode lvd_qdc_get_sc_rate(struct lvd_dev* dev, int addr, ems_u32* rate);

plerrcode lvd_qdc_sc_init(struct lvd_dev* dev, int addr, int, int);
plerrcode lvd_qdc_sc_clear(struct lvd_dev* dev, ems_i32 addr);
plerrcode lvd_qdc_sc_update(struct lvd_dev* dev);
plerrcode lvd_qdc_sc_get(struct lvd_dev* dev, int, ems_u32, ems_u32*, int*);

int lvd_cardstat_qdc(struct lvd_dev*, struct lvd_acard*, void*, int);
int lvd_cardstat_qdc80(struct lvd_dev*, struct lvd_acard*, void*, int);

#endif
