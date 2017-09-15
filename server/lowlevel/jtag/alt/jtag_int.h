/*
 * lowlevel/jtag/jtag_int.h
 * created 2005-Aug-03 PW
 * $ZEL: jtag_int.h,v 1.1 2006/09/15 17:54:01 wuestner Exp $
 */

#ifndef _jtag_int_h_
#define _jtag_int_h_

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>
#include <emsctypes.h>
#include "jtag_low.h"

#ifdef NEVER
#define BYPASS  0xFF
#define IDCODE  0xFE
#define ISPEN   0xE8    /* enable in-system programming mode */
#define ISPEX   0x??    /* exit in-system programming mode */
#define FPGM    0xEA    /* Programs specific bit values at specified addresses */
#define FADDR   0xEB    /* Sets the PROM array address register */
#define FVFY1   0xF8    /* Reads the fuse values at specified addresses */
#define NORMRST	0xF0    /* Exits ISP Mode ? */
#define FERASE  0xEC    /* Erases a specified program memory block */
#define SERASE  0x0A    /* Globally refines the programmed values in the array */
#define FDATA0  0xED    /* Accesses the array word-line register ? */
#define FDATA3  0xF3    /* 6 */
#define CONFIG  0xEE
#endif

#define VID_AMD              1 /* 0x1 */
#define VID_INTEL            9 /* 0x9 */
#define VID_DEC             53 /* 0x35 */
#define VID_XILINX          73 /* 0x49 */
#define VID_COMPAQ          74 /* 0x4A */
#define VID_ANALOG_DEVICES 101 /* 0x65 */
#define VID_ALTERA         110 /* 0x6E */
#define VID_VITESSE        116 /* 0x74 */
/* AMCC 112 (bank 2) */

#define DID_XC18V256      0x5022 /* 256 Kbit */
#define DID_XC18V512a     0x5023 /* 512 Kbit */
#define DID_XC18V512b     0x5033 /* "      " */
#define DID_XC18V01a      0x5024 /* 1 Mbit */
#define DID_XC18V01b      0x5034 /* "    " */
#define DID_XC18V02a      0x5025 /* 2 Mbit */
#define DID_XC18V02b      0x5035 /* "    " */
#define DID_XC18V04a      0x5026 /* 4 Mbit */
#define DID_XC18V04b      0x5036 /* "    " */
#define DID_XCF01S        0x5044 /* 1 Mbit */
#define DID_XCF02S        0x5045 /* 2 Mbit */
#define DID_XCF04S        0x5046 /* 4 Mbit */
#define DID_XCF08P        0x5057 /* 8 Mbit */
#define DID_XCF16P        0x5058 /* 16 Mbit */
#define DID_XCF32P        0x5059 /* 32 Mbit */
#define DID_XC2S100_FG256 0x0614
#define DID_XC2S150       0x0618 /* Spartan II; 150000 gates *//*XCV150*/
#define DID_XC2S200_BG256 0x061C
#define DID_XCV300        0x0620
#define DID_XCV400        0x0628
#define DID_XC2V2000      0x1038
#define DID_XC3S200       0x1414
#define DID_XC3S400       0x141c /* Spartan3 */
#define DID_XC3S1000      0x1428 /* Spartan3 */

struct jtag_chipdata {
    u_int32_t vendor_id;
    u_int32_t part_id;
    int ir_len;	          /* length of instruction register */
    int mem_size;
    int write_size;
    const char* name;
};

struct jtag_chain;
struct jtag_chip {
    struct jtag_chain* chain;
    u_int32_t id;
    const struct jtag_chipdata* chipdata;
    int version;
    int pre_c_bits;
    int after_c_bits;
    int pre_d_bits;
    int after_d_bits;
    ems_u32 ir;
};

enum jtag_states {
    jtag_TLR, /* test-logic-reset */
    jtag_RTI, /* run-test/idle */
    jtag_SDS, /* select-dr-scan */
    jtag_CD,  /* capture-dr */
    jtag_SD,  /* shift-dr */
    jtag_E1D, /* exit1-dr */
    jtag_PD,  /* pause-dr */
    jtag_E2D, /* exit2-dr */
    jtag_UD,  /* update-dr */
    jtag_SIS, /* select-ir-scan */
    jtag_CI,  /* capture-ir */
    jtag_SI,  /* shift-ir */
    jtag_E1I, /* exit1-ir */
    jtag_PI,  /* pause-ir */
    jtag_E2I, /* exit-ir */
    jtag_UI,  /* update-ir */
};

struct jtag_chain {
    struct jtag_dev jtag_dev;
    int num_chips;
    int valid;  /* ir_len of all chips known */
    enum jtag_states jstate;
    int jstate_count;
    int state;
    struct jtag_chip* chips;
};

extern int jtag_traced;

#endif
