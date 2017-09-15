/*
 * lvd_trig_map.h
 * $ZEL: lvd_trig_map.h,v 1.4 2011/04/15 19:50:20 wuestner Exp $
 * created 2009-Nov-10
 */

#ifndef _lvd_trig_map_h_
#define _lvd_trig_map_h_

#include <sys/types.h>

/* registers of a trigger card */
/* 128 Byte */
struct lvd_trig_card {
        u_int16_t           ident;      /*   0 */
        u_int16_t           serial;     /*   2 */
        u_int32_t           temp;       /*   4 */
        u_int32_t           ro_data;    /*   8 */
        u_int16_t           cr;         /*   c */
#define TRIG_CR_ENA       0x1
#define TRIG_CR_TRG_DATA  0x2
#define TRIG_CR_RAW_DATA  0x4
#define TRIG_CR_RAW_EXT   0x8
#define TRIG_CR_TRG_TST  0x10
        u_int16_t           sr;         /*   e */
#define TRIG_SR_SEQ     0x1
        u_int16_t           i2c_csr;    /*  10 */
        u_int16_t           i2c_data;   /*  12 */
        u_int16_t           i2c_wr;     /*  14 */
        u_int16_t           i2c_rd;     /*  16 */
        u_int16_t           i2c_sel;    /*  18 */
        u_int16_t           fw_ver;     /*  1a */
        u_int16_t           rx_sync;    /*  1c */
        u_int16_t           rm_id;      /*  1e */
        u_int16_t           res20[2];   /*  20 */
        union {
            u_int32_t       mem_data_l; /*  24 */
            u_int16_t       mem_data_s[2]; /*  24 */
        };
        u_int16_t           mem_select; /*  28 */
        u_int16_t           res2a;      /*  2a */
        union {
            u_int32_t       pi_data_l;  /*  2c */
            u_int16_t       pi_data_s[2]; /*  2c */
        };
        u_int16_t           pi_addr;    /*  30 */

        u_int16_t           res32[7];   /*  32 */
        u_int16_t           drp_addr;   /*  40 */
        u_int16_t           drp_data;   /*  42 */
        u_int16_t           tx_ena;     /*  44 */
        u_int16_t           res46;      /*  46 */
        u_int16_t           ddr_csr;    /*  48 */
        u_int16_t           ddr_len;    /*  4a */
        u_int32_t           ddr_data;   /*  4c */
        u_int32_t           ddr_addr;   /*  50 */
        u_int16_t           res54[18];  /*  54 */
        u_int32_t           jtag_data;  /*  78 */
        u_int16_t           jtag_csr;   /*  7c */
        u_int16_t           ctrl;       /*  7e */
};

#endif
