/*
 * lvd_f1_map.h
 * $ZEL: lvd_f1_map.h,v 1.8 2009/06/02 19:37:57 wuestner Exp $
 * created 2005-Feb-26
 */

#ifndef _lvd_f1_map_h_
#define _lvd_f1_map_h_

#include <sys/types.h>
/*#include <dev/pci/zellvd_map.h>*/

/*
 * F1 definitions
 */

#define F1_T_REF                25          /* 25 ns == 40 MHz */
#define F1_REFCLKDIV            7
#define F1_HSDIV                (F1_T_REF*(1<<F1_REFCLKDIV)/0.152)
#define F1_REFCNT               0x100       /* used for synchron mode */

#define F1_R1_HIRES             0x8000
#define F1_R1_HITM              0x4000
#define F1_R1_LETRA             0x2000
#define F1_R1_FAKE              0x0400
#define F1_R1_OVLAP             0x0200
#define F1_R1_M_IN              0x0020
#define F1_R1_DACSTRT           0x0001

#define F1_R7_BEINIT            0x8000

#define F1_R15_ROSTA            0x0008      /* ring oszillator start */
#define F1_R15_SYNC             0x0004
#define F1_R15_COM              0x0002
#define F1_R15_8BIT             0x0001

/* registers of a F1 TDC card */
/* 128 Byte */
struct lvd_f1_card {
        u_int16_t           ident;
        u_int16_t           serial;
        u_int16_t           trg_lat;
        u_int16_t           trg_win;
        u_int16_t           ro_data;
        u_int16_t           f1_addr;
#define F1_ALL              8

        u_int16_t	    cr;     /* 2 bit */
/* F1 FW version < 2.0 */
#define F1_OLD_CR_IDLE        0    /* disable DAQ */
#define F1_OLD_CR_RAW      0x01    /* ohne Zeitfenster */
#define F1_OLD_CR_TLIM     0x02    /* ohne Zeitfenster aber mit Zeitlimit */
#define F1_OLD_CR_LETRA    0x04
#define F1_OLD_CR_EXT_RAW  0x08 /* ?? extended raw mode (2 long word per hit)*/
/* F1 FW version >= 2.0 */
#define F1_OLD1_CR_IDLE            0    /* disable DAQ */
#define F1_OLD1_CR_TRIGG_MATCH     1    /* "normal" DAQ mode */
#define F1_OLD1_CR_RAW             2    /* without time window */
#define F1_OLD1_CR_TLIM            3    /* w/o time window but with time limit */
#define F1_OLD1_CR_LETRA           5    /* protect LSB against subtraction */
#define F1_OLD1_CR_EXT_RAW         6    /* extended raw mode (2 words per hit) */
/* F1 FW version >= 2.3 (or earlyer?) */
#define F1_CR_TMATCHING    0x01    /* trigger matching mode */
#define F1_CR_RAW          0x02    /* no subtraction of trigger time */
#define F1_CR_LETRA        0x04    /* edge identification in LSB */
#define F1_CR_EXTENDED     0x08    /* extended time in raw format */

        u_int16_t           sr;
#define F1_IN_FIFO_ERR      0x1     /* no double word */

        u_int16_t           f1_state[8];
#define F1_WRBUSY           0x01
#define F1_NOINIT           0x02
#define F1_NOLOCK           0x04
#define F1_HOFL             0x10    /* F1 Hit FIFO Overflow */
#define F1_OOFL             0x20    /* F1 Output FIFO Overflow */
#define F1_SEQERR           0x40    /* F1 data wrong (wrong sequence) */
#define F1_IOFL             0x80    /* FPGA input FIFO overflow */
#define F1_INPERR           (IOFL|SEQERR|OOFL|HOFL)

        u_int16_t           f1_regX[16];
        u_int16_t	    jtag_csr;
#define F1_JT_TDI           0x001
#define F1_JT_TMS           0x002
#define F1_JT_TDO           0x008
#define F1_JT_ENABLE        0x100
#define F1_JT_AUTO_CLOCK    0x200
#define F1_JT_SLOW_CLOCK    0x400
#define F1_JT_C             (F1_JT_ENABLE|F1_JT_AUTO_CLOCK)

        u_int16_t           res42;
        u_int32_t           jtag_data;

        u_int32_t           res48;
        u_int16_t           f1_range;
        u_int16_t           res4E;
        u_int16_t           hit_ovfl[4];
        u_int16_t           res58[19];
        u_int16_t           ctrl;
};

#endif
