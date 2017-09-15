/*
 * lowlevel/jtag/jtag_chipdata.c
 * created 2006-Jul-06 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_chipdata.c,v 1.6 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <rcs_ids.h>
#include "jtag_chipdata.h"

#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x
#define __SS(s) __STRING(s)
#define JTAG_DEV(vendor, name, ir_len, mem_size, write_size, erase, read, write) { \
    __CONCAT(VID_, vendor),                                                 \
    __CONCAT(DID_, name),                                                   \
    __SS(name),                                                             \
    ir_len,                                                                 \
    mem_size,                                                               \
    write_size,                                                             \
    erase,                                                                  \
    read,                                                                   \
    write                                                                   \
}
#define JTAG_DEV_END(name) {           \
    0, 0, __SS(name), -1, -1, -1, 0, 0, 0 \
}

#define VID_AMD              1 /* 0x1 */
#define VID_INTEL            9 /* 0x9 */
#define VID_DEC             53 /* 0x35 */
#define VID_XILINX          73 /* 0x49 */
#define VID_COMPAQ          74 /* 0x4A */
#define VID_ANALOG_DEVICES 101 /* 0x65 */
#define VID_ALTERA         110 /* 0x6E */
#define VID_VITESSE        116 /* 0x74 */
/* AMCC 112 (bank 2) */

static const struct jtag_chipdata jtag_chipdata[]={
    JTAG_DEV(XILINX, XC18V256,      8,  0x8000, -1, -1,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V512a,     8, 0x10000, -1, -1,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V512b,     8, 0x10000, -1, -1,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V01a,      8, 0x20000, 0x100, 1500000,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V01b,      8, 0x20000, 0x100, 1500000,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V02a,      8, 0x40000, 0x200, 1500000,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V02b,      8, 0x40000, 0x200, -1,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V04a,      8, 0x80000, 0x200, 1500000,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XC18V04b,      8, 0x80000, 0x200, 1500000,
        jtag_read_XC18V00, jtag_write_XC18V00),
    JTAG_DEV(XILINX, XCF01S,        8, 0x20000, 0x100, 1500000,
        0, jtag_write_XCFS),
    JTAG_DEV(XILINX, XCF02S,        8, 0x40000, -1, -1,
        0, jtag_write_XCFS),
    JTAG_DEV(XILINX, XCF04S,        8, 0x80000, 0x200, 1500000,
        jtag_read_XCFS, jtag_write_XCFS),
    JTAG_DEV(XILINX, XCF08P,       16, 0x100000, 0x020, 14000000,
        jtag_read_XCFP, jtag_write_XCFP),
    JTAG_DEV(XILINX, XCF16P,       16, 0x200000, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XCF32P,       16, 0x400000, 0x020, 14000000,
        jtag_read_XCFP, jtag_write_XCFP),
    JTAG_DEV(XILINX, XC2S100_FG256, 5, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XC2S150,       5, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XC2S200_BG256, 5, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XCV300,        5, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XCV400,        5, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XC2V2000,      6, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XC3S200,       6, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XC3S400,       6, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XC3S1000,      6, -1, -1, -1, 0, 0),
    JTAG_DEV(XILINX, XC4VFX20,     10, -1, -1, -1, 0, 0),
    JTAG_DEV_END(unknown device),
};

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
void
find_jtag_chipdata(struct jtag_chip* jtag_chip, int verbose)
{
    const struct jtag_chipdata* d=jtag_chipdata;

    jtag_chip->chipdata=0;
    while (d->vendor_id) {
        ems_u32 id=(d->part_id<<12)|(d->vendor_id<<1)|0x1;
        if ((jtag_chip->id&0x0fffffff)==id) {
            jtag_chip->chipdata=d;
            return;
        }
        d++;
    }
    if (verbose) {
        printf("Vendor 0x%x Device 0x%x (not found)\n",
                (jtag_chip->id>>1)&0x7ff,
                (jtag_chip->id>>12)&0xffff);
    }
}
/****************************************************************************/
/****************************************************************************/
