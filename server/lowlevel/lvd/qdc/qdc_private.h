/*
 * lowlevel/lvd/qdc/qdc_private.h
 * $ZEL: qdc_private.h,v 1.3 2017/10/20 23:21:31 wuestner Exp $
 * created 2012-Sep-11 PW
 */

#ifndef _lvd_qdc_private_h_
#define _lvd_qdc_private_h_

struct qdc_info {
    ems_u64 scaler_u[16];
    ems_u64 scaler_l[16];
    ems_u16 dac_shadow[16];
    ems_u16 threshold_shadow[16];
    ems_u16 base_shadow[16];
    ems_u16 thrh_shadow[16];
    ems_u16 scaler_rate_shadow;
    ems_u16 testdac_shadow;
    ems_u16 cr_shadow;
    ems_u16 cr_shadow_;
    float base_clk;
    float adc_clk;
    ems_u16 f1flag;
    int qdcver, qdcverL;
    u_int16_t dac_shadow_valid;
    u_int16_t threshold_shadow_valid;
    u_int16_t base_shadow_valid;
    u_int16_t thrh_shadow_valid;
    u_int16_t scaler_rate_shadow_valid;
    u_int16_t testdac_shadow_valid;
    int scaler_enabled;
    int scaler_first;
    int scaler_last;
};

int lvd_cardstat_qdc(struct lvd_dev*, struct lvd_acard*, void*, int);
int lvd_cardstat_qdc80(struct lvd_dev*, struct lvd_acard*, void*, int);
int lvd_qdc_cr_sel(struct lvd_acard* acard, ems_u32 sel);
int lvd_qdc_cr_rev(struct lvd_acard* acard);

#endif
