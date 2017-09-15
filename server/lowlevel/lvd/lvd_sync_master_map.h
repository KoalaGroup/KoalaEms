/*
 * lvd_sync_master_map.h
 * $ZEL: lvd_sync_master_map.h,v 1.6 2009/06/02 19:30:57 wuestner Exp $
 * created 2006-Feb-07
 */

#ifndef _lvd_sync_master_map_h_
#define _lvd_sync_master_map_h_

#include <sys/types.h>

/*
 * sync_master definitions
 */

struct lvd_msync_card {
        u_int16_t           ident;         /* 00 */
        u_int16_t           serial;        /* 02 */
#if 0
        u_int16_t           sr1;           /* 04 */
#define MSYNC_SR1_BUSY_OR     0x1
#define MSYNC_SR1_BUSY_AND    0x2
#define MSYNC_SR1_SEQ_ERROR   0x4
#else
        u_int16_t           res04;
#endif
        u_int16_t           res06;         /* 06 */
        u_int16_t           ro_data;       /* 08 */
        u_int16_t           res0a;         /* 0a */
        u_int16_t	    cr;            /* 0c */
#define MSYNC_CR_RING        0x0001
#define MSYNC_CR_GO          0x0002
#define MSYNC_CR_TAW_ENA     0x0004
#define MSYNC_CR_AUX         0x00f0
#define MSYNC_CR_T4LATCH     0x0f00
#define MSYNC_CR_LOCBUSY     0x1000

        u_int16_t           sr;            /* 0e */
#define MSYNC_SR_RING_TRG    0x000f
#define MSYNC_SR_RING_GO     0x0010
#define MSYNC_SR_VETO        0x0020
#define MSYNC_SR_SI          0x0040
#define MSYNC_SR_M_INH       0x0080
#define MSYNC_SR_AUX_IN      0x0f00
#define MSYNC_SR_RING_SI     0x2000
#define MSYNC_SR_HL_TOUT     0x4000
#define MSYNC_SR_RING_TDT    0x8000

        u_int16_t           trig_inhibit;  /* 10 */
        u_int16_t           log_inhibit;   /* 12 */
        u_int16_t           trig_input;    /* 14 */
        u_int16_t           trig_accepted; /* 16 */
        u_int16_t           res18;         /* 18 */
        u_int16_t           fast_clr;      /* 1a */
        u_int16_t           res1c;         /* 1c */
        u_int16_t           res1e;         /* 1e */
        u_int16_t           sel_pw[8];     /* 20 */
        u_int16_t           sel_xpw;       /* 30 */
        u_int16_t	    pw_ctrl;       /* 32 */
        u_int32_t           timer;         /* 34 */
        u_int16_t           res38;
        u_int16_t           res3a;
        u_int32_t           tdt;           /* 3c */
        u_int16_t           jtag_csr;      /* 40 */
#define MSYNC_JT_TDI          0x001
#define MSYNC_JT_TMS          0x002
#define MSYNC_JT_TDO          0x008
#define MSYNC_JT_ENABLE       0x100
#define MSYNC_JT_AUTO_CLOCK   0x200
#define MSYNC_JT_SLOW_CLOCK   0x400
#define MSYNC_JT_C            (MSYNC_JT_ENABLE|MSYNC_JT_AUTO_CLOCK)
        u_int16_t           res42;
        u_int32_t           jtag_data;     /* 44 */
        u_int16_t           res48;
        u_int16_t           fcat;          /* 4a */
        u_int32_t           evc;           /* 4c */
        u_int16_t           res50[23];     /* 50 */
        u_int16_t           ctrl;          /* 7e */
};

struct lvd_msync2_card {
        u_int16_t           ident;         /* 00 */
        u_int16_t           serial;        /* 02 */
        u_int16_t           res04;         /* 04 */
        u_int16_t           res06;         /* 06 */
        u_int16_t           ro_data;       /* 08 */
        u_int16_t           res0a;         /* 0a */
        u_int16_t	    cr;            /* 0c */
#define MSYNC2_CR_RUN         0x0001
#define MSYNC2_CR_LBUSY       0x0002
#define MSYNC2_CR_TAW_ENA     0x0004
#define MSYNC2_CR_PW          0x0008
#define MSYNC2_CR_LATCH       0x0010
#define MSYNC2_CR_AUX_OUT0    0x0040
#define MSYNC2_CR_AUX_OUT1    0x0080
#define MSYNC2_CR_AUX_OUT     0x00C0
        u_int16_t           sr;            /* 0e */
#define MSYNC2_SR_FERR        0x0001
#define MSYNC2_SR_RUN         0x0002
#define MSYNC2_SR_ACTIVE      0x0004
#define MSYNC2_SR_BUSY        0x0008
#define MSYNC2_SR_BSYALL      0x0010
#define MSYNC2_SR_VETO        0x0020
#define MSYNC2_SR_AUX_IN0     0x0040
#define MSYNC2_SR_AUX_IN1     0x0080
#define MSYNC2_SR_AUX_IN      0x00C0
        u_int16_t           trig_inhibit;  /* 10 */
        u_int16_t           sum_inhibit;   /* 12 */
        u_int16_t           log_inhibit;   /* 14 */
        u_int16_t           trig_input;    /* 16 */
        u_int16_t           trig_accepted; /* 18 */
        u_int16_t           res1a[13];     /* 1a-32 */
        u_int32_t           timer;         /* 34 */
        u_int16_t           res38[2];      /* 38-3a */
        u_int32_t           tdt;           /* 3c */
        u_int16_t           jtag_csr;      /* 40 */
        u_int16_t           res42;         /* 42 */
        u_int32_t           jtag_data;     /* 44 */
        u_int16_t           res48[2];      /* 48-4a */
        u_int32_t           evc;           /* 4c */
        u_int16_t           res50[23];     /* 50 */
        u_int16_t           ctrl;          /* 7e */
#define MSYNC2_CTRL_MRESET   0x0001
#define MSYNC2_CTRL_TRG_RST  0x0002
#define MSYNC2_CTRL_SYNC_RES 0x0004
#define MSYNC2_CTRL_TRG_MRST 0x0080
};

#endif
