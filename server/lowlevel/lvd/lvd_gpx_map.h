/*
 * lvd_gpx_map.h
 * $ZEL: lvd_gpx_map.h,v 1.7 2008/03/11 20:13:27 wuestner Exp $
 * created 2006-Feb-07
 */

#ifndef _lvd_gpx_map_h_
#define _lvd_gpx_map_h_

#include <sys/types.h>

/*
 * GPX definitions
 */
#define GPX_T_REF                25          /* 40 MHz */
#define GPX_REFCLKDIV            7
#define GPX_HSDIV                (GPX_T_REF*(1<<GPX_REFCLKDIV)/0.216)

/* registers of a GPX TDC card */
/* 128 Byte */
struct lvd_gpx_card {
        u_int16_t           ident;
        u_int16_t           serial;
        u_int16_t           trg_lat;
        u_int16_t           trg_win;
        u_int16_t           ro_data;
        u_int16_t           res0a;
        u_int16_t	    cr;     /* 2 bit */
#define GPX_CR_TMATCHING    0x01    /* trigger matching mode */
#define GPX_CR_RAW          0x02    /* no subtraction of trigger time */
#define GPX_CR_LETRA        0x04    /* edge identification in LSB */
#define GPX_CR_EXTENDED     0x08    /* extended time in raw format */
#define GPX_CR_GPXMODE      0x10    /* native GPX data */

        u_int16_t           sr;
#define GPX_SR_SEQ_ERR      0x1     /* no double word */
#define GPX_SR_DAC_BUSY     0x2     /* DAC programming in progress */

        u_int16_t           gpx_int_err;
#define GPX_ERRFL           0xff
#define GPX_INTFL           0xff00

        u_int16_t           gpx_empty;
        u_int16_t           dcm_shift;
        u_int16_t           res12[5];
        u_int32_t           gpx_data;
        u_int16_t           gpx_seladdr;
        u_int16_t           dac_data;
        u_int16_t           res26[4+1*8];
        u_int16_t	    jtag_csr;
#define GPX_JT_TDI          0x001
#define GPX_JT_TMS          0x002
#define GPX_JT_TDO          0x008
#define GPX_JT_ENABLE       0x100
#define GPX_JT_AUTO_CLOCK   0x200
#define GPX_JT_SLOW_CLOCK   0x400
#define GPX_JT_C            (GPX_JT_ENABLE|GPX_JT_AUTO_CLOCK)

        u_int16_t           res42;
        u_int32_t           jtag_data;

        u_int16_t           res48[2];
        u_int32_t           gpx_range;
        u_int16_t           res50[2*8+7];
        u_int16_t           ctrl;
};

#endif
