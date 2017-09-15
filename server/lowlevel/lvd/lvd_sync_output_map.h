/*
 * lvd_sync_output_map.h
 * $ZEL: lvd_sync_output_map.h,v 1.7 2009/04/01 15:56:47 wuestner Exp $
 * created 2006-Feb-07
 */

#ifndef _lvd_sync_output_map_h_
#define _lvd_sync_output_map_h_

#include <sys/types.h>

/*
 * sync_output definitions
 */

struct lvd_osync_card {
        u_int16_t           ident;         /* 00 */
        u_int16_t           serial;        /* 02 */
        u_int16_t           bsy_tmo;       /* 04 */
        u_int16_t           busy;          /* 06 */
#define OSYNC_BUSY_LOCAL 0x00ff
#define OSYNC_BUSY_INPUT 0xff00
        u_int16_t           ro_data;       /* 08 */
        u_int16_t           res0a;
        u_int16_t	    cr;            /* 0c */
#define OSYNC_CR_TENA     0x00ff
#define OSYNC_CR_RUN      0x0100
#define OSYNC_CR_PULSE    0xff00

        u_int16_t           sr;            /* 0e */
#define OSYNC_SR_BUSY_IN   0x0ff
#define OSYNC_SR_TRG_SUM   0x100
#define OSYNC_SR_SEQ_ERROR 0x200

        u_int16_t           trg_in;        /* 10 */
        u_int16_t           sr_addr;       /* 12 */
        u_int16_t           sr_data;       /* 14 */
        u_int16_t           res16;         /* 16 */
        u_int16_t           tpat;          /* 18 */
        u_int16_t           res1a;         /* 1a */
        u_int32_t           evc;           /* 1c */
        u_int16_t           res1e[16];     /* 20 */
        u_int16_t	    jtag_csr;      /* 40 */
#define OSYNC_JT_TDI          0x001
#define OSYNC_JT_TMS          0x002
#define OSYNC_JT_TDO          0x008
#define OSYNC_JT_ENABLE       0x100
#define OSYNC_JT_AUTO_CLOCK   0x200
#define OSYNC_JT_SLOW_CLOCK   0x400
#define OSYNC_JT_C            (OSYNC_JT_ENABLE|OSYNC_JT_AUTO_CLOCK)
        u_int16_t           res42;
        u_int32_t           jtag_data;     /* 44 */
        u_int16_t           res48[27];     /* 48 */
        u_int16_t           ctrl;          /* 7e */
};

#endif
