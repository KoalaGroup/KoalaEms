/*
 * lowlevel/jtag_low.c
 * created 12.Aug.2005 PW
 * $ZEL: jtag_low.c,v 1.2 2006/11/08 15:30:23 wuestner Exp $
 */

#error NOT USED!!!

#include <sconf.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include "../devices.h"
#include "jtag_low.h"
#include "jtag_int.h"

#ifdef LOWLEVEL_UNIXVME
#include "../unixvme/vme.h"
#endif
#ifdef LOWLEVEL_LVD
#include "../lvd/lvdbus.h"
#endif

#define __CONCAT(x,y)	x ## y
#define __STRING(x)	#x
#define __SS(s) __STRING(s)
#define JTAG_DEV(vendor, name, ir_len, mem_size, write_size) { \
    __CONCAT(VID_, vendor),                                    \
    __CONCAT(DID_, name),                                      \
    ir_len,                                                    \
    mem_size,                                                  \
    write_size,                                                \
    __SS(name)                                                 \
}
#define JTAG_DEV_END(name) {                                   \
    0, 0, -1, -1, -1, __SS(name)                               \
}

static const struct jtag_chipdata jtag_chipdata[]={
    JTAG_DEV(XILINX, XC18V01 , 8, 0x20000, 0x100),
    JTAG_DEV(XILINX, XC18V02 , 8, 0x40000, 0x200),
    JTAG_DEV(XILINX, XC18V04a, 8, 0x80000, 0x200),
    JTAG_DEV(XILINX, XC18V04b, 8, 0x80000, 0x200),
    JTAG_DEV(XILINX, XC2V2000, 6, -1, -1),
    JTAG_DEV(XILINX, XC3S200 , 6, -1, -1),
    JTAG_DEV(XILINX, XCF01S  , 8, -1, -1),
    JTAG_DEV(XILINX, XCV150  , 5, -1, -1),
    JTAG_DEV(XILINX, XCV300  , 5, -1, -1),
    JTAG_DEV(XILINX, XCV400  , 5, -1, -1),
    JTAG_DEV_END(unknown device),
};

static enum jtag_states jtag_transitions[][2]={
    /*jtag_TLR*/ {jtag_RTI, jtag_TLR},
    /*jtag_RTI*/ {jtag_RTI, jtag_SDS},
    /*jtag_SDS*/ {jtag_CD , jtag_SIS},
    /*jtag_CD */ {jtag_SD , jtag_E1D},
    /*jtag_SD */ {jtag_SD , jtag_E1D},
    /*jtag_E1D*/ {jtag_PD , jtag_UD},
    /*jtag_PD */ {jtag_PD , jtag_E2D},
    /*jtag_E2D*/ {jtag_SD , jtag_UD},
    /*jtag_UD */ {jtag_RTI, jtag_SDS},
    /*jtag_SIS*/ {jtag_CI , jtag_TLR},
    /*jtag_CI */ {jtag_SI , jtag_E1I},
    /*jtag_SI */ {jtag_SI , jtag_E1I},
    /*jtag_E1I*/ {jtag_PI , jtag_UI},
    /*jtag_PI */ {jtag_PI , jtag_E2I},
    /*jtag_E2I*/ {jtag_SI , jtag_UI},
    /*jtag_UI */ {jtag_RTI, jtag_SDS},
};
/*
static const char* jtag_state_names[]={
    "test-logic-reset",
    "run-test/idle",
    "select-dr-scan",
    "capture-dr",
    "shift-dr",
    "exit1-dr",
    "pause-dr",
    "exit2-dr",
    "update-dr",
    "select-ir-scan",
    "capture-ir",
    "shift-ir",
    "exit1-ir",
    "pause-ir",
    "exit2-ir",
    "update-ir",
};
*/

/****************************************************************************/
#if 0
static void
printbits32(ems_u32 d, const char* as, const char* es)
{
    int i;

    printf("%s", as);
    for (i=0; i<32; i++) {
        int bit=!!(d&0x80000000UL);
        printf("%d", bit);
        if (i%4==3)
            printf(" ");
        d<<=1;
    }
    printf("%s", es);
}
#endif
/****************************************************************************/
static void
changestate(struct jtag_chain* chain, int tms)
{
    enum jtag_states oldstate;

    if ((tms<0) || (tms>1)) {
        printf("changestate: invalid ms: %d\n", tms);
        return;
    }
    oldstate=chain->jstate;
    chain->jstate=jtag_transitions[oldstate][tms];
}
/****************************************************************************/
static int
jtag_action(struct jtag_chain* chain, int tms, int tdi, int* tdo)
{
    struct jtag_dev* jtag_dev=&chain->jtag_dev;
    int tdo__, res;
#if 0
    tdi=!!tdi;
#else
    tdi=tdi==1;
#endif

    res=jtag_dev->jt_action(jtag_dev, tms, tdi, &tdo__);
    if (tdo)
        *tdo=tdo__;
    changestate(chain, tms);
    return res;
}
/****************************************************************************/
static int
jtag_data(struct jtag_chain* chain, u_int32_t* d)
{
    struct jtag_dev* jdev=&chain->jtag_dev;
    jdev->jt_data(jdev, d);
    return 0;
}
/****************************************************************************/
static int
jtag_data32(struct jtag_chip* chip, u_int32_t din, u_int32_t* dout, int len)
{
    struct jtag_chain* chain=chip->chain;
    int i;

    /*printf("data32(in=%x len=%d)\n", din, len);*/

    jtag_action(chain, 1, 2, 0);
    jtag_action(chain, 0, 2, 0);
    jtag_action(chain, 0, 2, 0);			/* SHIFT-DR */

    /* shift pre_bits */
    for (i=chip->pre_d_bits; i>0; i--) {
        jtag_action(chain, 0, 2, 0);
    }

    /* shift len-1 data bits */
    for (i=len-1; i>0; i--) {
        jtag_action(chain, 0, din&1, 0);
        din>>=1;
    }
    /* read data */
    if (dout) {
        ems_u32 v;
        jtag_data(chain, &v);
        v>>=32-len;
        *dout=v;
        /*printbits32(v,  "data ", "\n");*/
    }

    /* if (after_bits>1): shift last data bit and after_bits-1 */
    for (i=chip->after_d_bits-1; i>=0; i--) {
        jtag_action(chain, 0, din&1, 0);   /* SD */
    }
    /* shift either last data bit or last afterbit */
    jtag_action(chain, 1, din&1, 0);       /* E1D */

    jtag_action(chain, 1, 2, 0);        /* UD */
    jtag_action(chain, 0, 2, 0);	/* state RTI */

    return 0;
}
/****************************************************************************/
static int
jtag_rd_data(struct jtag_chip* chip, u_int32_t *buf, int len)
{
    struct jtag_chain* chain=chip->chain;
    int i, j, k;

    if (len == 0) {
        if (chain->state == 1) {
            jtag_action(chain, 1, 2, 0);
            jtag_action(chain, 1, 2, 0);
            jtag_action(chain, 0, 2, 0);
            chain->state=0;                          /* state RTI */
        }
        return 0;
    }

    if (chain->state == 0) {
        jtag_action(chain, 1, 2, 0);
        jtag_action(chain, 0, 2, 0);         /* CAPTURE-DR */

        for (i=chip->pre_d_bits; i>0; i--) {
            jtag_action(chain, 0, 1, 0);
        }
        chain->state=1;
    }

    for (i=(len+3)/4, k=0; i; i--, k++) {
        u_int32_t d;
        for (j=32; j; j--)
            jtag_action(chain, 0, 2, 0);
        jtag_data(chain, &d);
        *buf++ =d;
        if (k<10)
            printf("%08x\n", d);
    }
    return 0;
}
/****************************************************************************/
static int
jtag_wr_data(struct jtag_chip* chip, void *buf, int len, int end)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t *bf, din;
    u_int8_t ms, ms1;
    int i;

    bf=(u_int32_t*)buf;
    len >>=2;
    if (len ==0) return 0;

    jtag_action(chain, 1, 1, 0);
    jtag_action(chain, 0, 1, 0);
    jtag_action(chain, 0, 1, 0);

    for (i=chip->pre_d_bits; i>0; i--) {
        jtag_action(chain, 0, 1, 0);
    }

    ms=0;
    ms1=chip->after_d_bits?0:1;
    while (len) {
        din=*bf++;
        for (i=31; i>=0; i--) {
            if ((len==0) && !i) ms=ms1;
            jtag_action(chain, ms, din&1, 0);
            din>>=1;
        }
        len--;
    }

    for (i=chip->after_d_bits-1; i>=0; i--) {
        if (!i) ms=1;
        jtag_action(chain, ms, 1, 0);
    }

    if (end < 0) {
        jtag_action(chain, 1, 1, 0);
        jtag_action(chain, 0, 1, 0);
        chain->state=0;
    }

    return 0;
}
/****************************************************************************/
static int
jtag_instruction(struct jtag_chip* chip, u_int32_t icode, u_int32_t* ret)
{
    struct jtag_chain* chain=chip->chain;
    int i;

    /*printf("instruction(icode=%x)\n", icode);*/
    jtag_action(chain, 1, 2, 0);
    jtag_action(chain, 1, 2, 0);
    jtag_action(chain, 0, 2, 0);
    jtag_action(chain, 0, 2, 0);

    /* shift pre_bits */
    for (i=chip->pre_c_bits; i>0; i--) {
        jtag_action(chain, 0, 1, 0);
    }

    /* shift ir_len-1 instruction bits */
    for (i=chip->chipdata->ir_len-1; i>0; i--) {
        jtag_action(chain, 0, icode&1, 0);
        icode>>=1;
    }
    if (ret) {
        ems_u32 v;
        jtag_data(chain, &v);
        v>>=(32-chip->chipdata->ir_len);
        /*printbits32(v, "v ", "\n");*/
        *ret=v;
    }

    if (chip->after_c_bits==0) {
        /* shift last instruction bit */
        jtag_action(chain, 1, icode&1, 0); /* E1I */
    } else {
        /* shift last instruction bit */
        jtag_action(chain, 0, icode&1, 0); /* SI*/
        /* shift after_bits-1 afterbits */
        for (i=chip->after_c_bits-1; i>0; i--) {
            jtag_action(chain, 0, 1, 0);   /* SI*/
        }
        /* shift last afterbit */
        jtag_action(chain, 1, 1, 0);       /* E1I */
    }

    jtag_action(chain, 1, 1, 0); /* UI */
    if (icode == CONFIG)
        sleep(3);

    jtag_action(chain, 0, 1, 0); /* RTI */

    return 0;
}
/****************************************************************************/
int
jtag_read_XC18V00(struct jtag_chip* chip, u_int8_t* data)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t dout, size, ret;
    int i;

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    printf("reading device %s\n", chip->chipdata->name);

    if (jtag_instruction(chip, IDCODE, 0)) {
        return -1;
    }
    if (jtag_data32(chip, -1, &dout, 32)<0)
        return -1;
    if ((dout&0x0fffffff)!=chip->id) {
        printf("wrong IDCODE: %08X\n", dout);
        return -1;
    }

    size=chip->chipdata->mem_size;

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x34, 0, 6);

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11) {
        if ((ret&~0x18)!=0x1) {
            printf("illegal capture pattern 0x%x\n", ret);
            return -1;
        } else {
            if (!(ret&0x10))
                printf("chip not in ISP status\n");
            if (ret&0x8)
                printf("security bit set\n");
            return -1;
        }
    }

    jtag_data32(chip, 0, 0, 16);
    jtag_instruction(chip, FVFY1, 0);

    for (i=0; i<50; i++) {      /* mindestens 20 */
        ems_u32 v;
        jtag_data(chain, &v);
        dout +=v;
    }

    if ((ret=jtag_rd_data(chip, (u_int32_t*)data, size)) != 0) {
        printf("\n");
        return ret;
    }
    jtag_rd_data(chip, 0, 0);

    return 0;
}
/****************************************************************************/
int
jtag_write_XC18V00(struct jtag_chip* chip, u_int8_t* data)
{
    struct jtag_chain* chain=chip->chain;
    u_int32_t dout, ret;
    int	size, len, i;

    if (chain->state) {
        printf("chain.state=%d\n", chain->state);
        return -1;
    }

    printf("writing device %s\n", chip->chipdata->name);

    size=chip->chipdata->mem_size;
    len=chip->chipdata->write_size;
    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x04, 0, 6);

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11) {
        printf("FADDR.0 %02X\n", ret);
        return -1;
    }
    jtag_data32(chip, 1, 0, 16);
    jtag_instruction(chip, FERASE, 0);

    for (i=0; i<1000; i++) {   /* mindestens 500 */
        ems_u32 v;
        jtag_data(chain, &v);
        dout +=v;
    }

    jtag_instruction(chip, NORMRST, 0);

    jtag_instruction(chip, BYPASS, &ret);
    if (ret!=0x01)
        printf("BYPASS.0 %02X\n", ret);

    jtag_instruction(chip, ISPEN, 0);
    jtag_data32(chip, 0x04, 0, 6);


    for (i=0; i<size/len; i++, data+=len) {
        int j;
        jtag_instruction(chip, FDATA0, 0);
        if (jtag_wr_data(chip, data, len, -1))
            return -1;

        if (!i) {
            jtag_instruction(chip, FADDR, &ret);
            if (ret!=0x11)
                printf("FADDR.1 %02X\n", ret);
            jtag_data32(chip, 0, 0, 16);
        }

        jtag_instruction(chip, FPGM, 0);

        for (j=0; j<1000; j++) {   /* mindestens 500 */
            ems_u32 v;
            jtag_data(chain, &v);
            dout +=v;
        }
        printf("."); fflush(stdout);
    }
    printf("\n");

    jtag_instruction(chip, FADDR, &ret);
    if (ret!=0x11)
        printf("FADDR.2 %02X\n", ret);

    jtag_data32(chip, 1, 0, 16);
    jtag_instruction(chip, SERASE, 0);

    for (i=0; i<1000; i++) {   /* mindestens 500 */
        ems_u32 v;
        jtag_data(chain, &v);
        dout +=v;
    }

    jtag_instruction(chip, NORMRST, 0);

    jtag_instruction(chip, BYPASS, &ret);
    if (ret!=0x01)
        printf("BYPASS.1 %02X\n", ret);

    printf("%d bytes written\n", size);

    return 0;
}
/****************************************************************************/
/****************************************************************************/
