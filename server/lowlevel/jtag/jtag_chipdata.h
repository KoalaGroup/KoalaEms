/*
 * lowlevel/jtag/jtag_chipdata.h
 * created 2006-Jul-07 PW
 * $ZEL: jtag_chipdata.h,v 1.3 2008/03/11 20:26:08 wuestner Exp $
 */

#ifndef _jtag_chipdata_h_
#define _jtag_chipdata_h_

#include <sconf.h>
#include <debug.h>

#include "jtag_int.h"

int jtag_read_XC18V00(struct jtag_chip*, ems_u8*, ems_u32*);
int jtag_write_XC18V00(struct jtag_chip*, ems_u8*, ems_u32);
int jtag_read_XCFS(struct jtag_chip* chip, ems_u8*, ems_u32*);
int jtag_write_XCFS(struct jtag_chip* chip, ems_u8*, ems_u32);
int jtag_read_XCFP(struct jtag_chip* chip, ems_u8*, ems_u32*);
int jtag_write_XCFP(struct jtag_chip* chip, ems_u8*, ems_u32);

void find_jtag_chipdata(struct jtag_chip*, int verbose);

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
#define DID_XC4VFX20      0x1E64 /* Virtex4 */

#endif
