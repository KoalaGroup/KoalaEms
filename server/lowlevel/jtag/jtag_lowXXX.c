/*
 * lowlevel/jtag_low.c
 * created 12.Aug.2005 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_lowXXX.c,v 1.2 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
#include <rcs_ids.h>
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
#define JTAG_DEV(vendor, name, ir_len, mem_size) { \
    __CONCAT(VID_, vendor),                        \
    __CONCAT(DID_, name),                          \
    ir_len,                                        \
    mem_size,                                      \
    __SS(name)                                     \
}
#define JTAG_DEV_END(name) {                       \
    0, 0, -1, -1, __SS(name)                       \
}

static const struct jtag_chipdata jtag_chipdata[]={
    JTAG_DEV(XILINX, XC18V01 , 8, 0x20000),
    JTAG_DEV(XILINX, XC18V02 , 8, 0x40000),
    JTAG_DEV(XILINX, XC18V04a, 8, 0x80000),
    JTAG_DEV(XILINX, XC18V04b, 8, 0x80000),
    JTAG_DEV(XILINX, XC2V2000, 6, -1),
    JTAG_DEV(XILINX, XC3S200 , 6, -1),
    JTAG_DEV(XILINX, XCF01S  , 8, -1),
    JTAG_DEV(XILINX, XCV150  , 5, -1),
    JTAG_DEV(XILINX, XCV300  , 5, -1),
    JTAG_DEV(XILINX, XCV400  , 5, -1),
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

const char* jtag_state_names[]={
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

int trace=0;

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
#if 0
static void
printbits64(ems_u64 d, int skip, const char* as, const char* es)
{
    int i;

    printf("%s", as);
    for (i=0; i<64; i++, skip--) {
        int bit=!!(d&0x8000000000000000ULL);
        if (skip>0)
            printf(" ");
        else
            printf("%d", bit);
        d<<=1;
    }
    printf("%s", es);
}
#endif
/****************************************************************************/
#ifdef LVD_JTAG_TRACE
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
find_jtag_chipdata(struct jtag_chip* jtag_chip)
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
    printf("Vendor 0x%x Device 0x%x", (jtag_chip->id>>1)&0x7ff,
            (jtag_chip->id>>12)&0xffff);
}
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
jtag_action(struct jtag_chain* chain, int tms, int tdi, int* tdo, int loop)
{
#ifdef LVD_JTAG_TRACE
    static ems_u32 ichain=0;
    static ems_u32 ochain=0;
    static char dchain[64];

#endif
    struct jtag_dev* jtag_dev=&chain->jtag_dev;
    enum jtag_states old_state;
    int tdo_, res;

    old_state=chain->jstate;
    res=chain->jtag_dev.jt_action(jtag_dev, tms, tdi, &tdo_);
    if (tdo)
        *tdo=tdo_;
    changestate(chain, tms);

    if (old_state==jtag_SI) {
        int bit=tdi, i;
        for (i=0; i<chain->num_chips; i++) {
            int nextbit=chain->chips[i].ir&1;
            chain->chips[i].ir>>=1;
            chain->chips[i].ir|=(bit<<(chain->chips[i].chipdata->ir_len-1));
            bit=nextbit;
        }
        if (chain->jstate!=jtag_SI) {
            int i;
            for (i=0; i<chain->num_chips; i++) {
                printf("IR[%d]: 0x%x\n", i, chain->chips[i].ir);
            }
        }
    }

    if (old_state==jtag_SD) {
        int i;
        for (i=63; i>0; i--)
            dchain[i]=dchain[i-1];
        dchain[0]=tdi?'1':'0';
        if (chain->jstate!=jtag_SD) {
            printf("DR: ");
            for (i=0; i<64; i++) {
                printf("%c", dchain[i]);
            }
            printf("\n");
        }
    } else if (chain->jstate==jtag_SD) {
        int i;
        for (i=0; i<64; i++)
            dchain[i]='_';
    }

#ifdef LVD_JTAG_TRACE
    ichain>>=1;
    ochain>>=1;
    if (tdi)  ichain|=0x80000000UL;
    if (tdo_) ochain|=0x80000000UL;

    if (trace) {
        if (trace>1) {
            ems_u32 data;

            chain->jtag_dev.jt_data(jtag_dev, &data);

            printf("%16s tms %d tdi %d %16s tdo %d loop %d\n",
                jtag_state_names[old_state],
                tms, tdi,
                jtag_state_names[chain->jstate],
                tdo_,
                loop);

            printbits32(ichain, "ichain ", " "); printf("%08x\n", ichain);
            printbits32(ochain, "ochain ", " "); printf("%08x\n", ochain);
            printbits32(data  , "data   ", " "); printf("%08x\n", data);
        }

        if (chain->jstate!=old_state) {
            printf("(%2d) %16s --> %16s\n", chain->state_count,
                    jtag_state_names[old_state],
                    jtag_state_names[chain->jstate]);
            chain->state_count=1;
        } else {
            chain->state_count++;
        }
    }
#endif
    return res;
}
/****************************************************************************/
static int
jtag_data(struct jtag_chain* chain, ems_u32* data)
{
    int res;
    struct jtag_dev* jtag_dev=&chain->jtag_dev;
    res=chain->jtag_dev.jt_data(jtag_dev, data);
    return res;
}
/****************************************************************************/
static int
jtag_goto_RTI(struct jtag_chain* chain)
{
    switch (chain->jstate) {
    case jtag_SD:
        if (jtag_action(chain, 1, 0, 0, -1)) return -1; /* E1D */
        if (jtag_action(chain, 1, 0, 0, -1)) return -1; /* UD */
        if (jtag_action(chain, 0, 0, 0, -1)) return -1; /* RTI */
        break;
    default:
        printf("jtag_goto_RTI: unexpected state %s\n",
                jtag_state_names[chain->jstate]);
        return -1;
    }
    return 0;
}
/****************************************************************************/
static int
jtag_force_TLR(struct jtag_chain* chain)
{
    int i;

    printf("force_TLR\n");
    chain->jstate=jtag_TLR; /* not known yet, but forced by the following loop*/
    for (i=0; i<5; i++) {                                /* force TLR state */
        if (jtag_action(chain, 1, 0, 0, i)) return -1;
    }
    return 0;
}
/****************************************************************************/
/*
 * state before: RTI or UI or SDS
 * state after:  RTI
 * reads 'bits' bits of data from 'chip'
 */
static int
jtag_rd_data32(struct jtag_chip* chip, int bits, ems_u32* data)
{
    struct jtag_chain* chain=chip->chain;
    int i;

    if (bits<1) {
        printf("jtag_wr_data32: bits=%d (illegal)\n", bits);
        return -1;
    }

    switch (chain->jstate) {
    case jtag_RTI:
    case jtag_UI:
        if (jtag_action(chain, 1, 0, 0, -1)) return -1; /* SDS */
        /* no break */
    case jtag_SDS:
        if (jtag_action(chain, 0, 0, 0, -1)) return -1; /* CD */
        break;
    default:
        printf("jtag_rd_data32: unexpected state %s\n",
                jtag_state_names[chain->jstate]);
        return -1;
    }

    for (i=chip->pre_d_bits; i>0; i--) {
        /* SD shift data from trailing chips away */
        if (jtag_action(chain, 0, 0, 0, i)) return -1;
    }
    for (i=bits-1; i>0; i--) {
        if (jtag_action(chain, 0, 0, 0, i)) return -1; /* SD shift data */
    }
    if (jtag_action(chain, 1, 0, 0, -1)) return -1;     /* E1D */
    if (jtag_data(chain, data)) return -1;
    (*data)>>=32-bits;

    if (jtag_action(chain, 1, 0, 0, -1)) return -1;     /* UD */
    if (jtag_action(chain, 0, 0, 0, -1)) return -1;     /* RTI */

    return 0;
}
/****************************************************************************/
static int
jtag_rd_data(struct jtag_chip* chip, int bytes, ems_u32* data)
{
    struct jtag_chain* chain=chip->chain;
    int i, j, count=0;

    printf("jtag_rd_data: coming from %s\n", jtag_state_names[chain->jstate]);

    switch (chain->jstate) {
    case jtag_RTI:
        if (jtag_action(chain, 1, 0, 0, -1)) return -1; /* SDS */
        /* no break */
    case jtag_SDS:
        if (jtag_action(chain, 0, 0, 0, -1)) return -1; /* CD */

        for (i=chip->pre_d_bits; i>0; i--) {
            if (jtag_action(chain, 0, 0, 0, i)) return -1; /* SD */
        }
        break;
    case jtag_SD:
        /* nothing to do */
        break;
    default:
        printf("jtag_rd_data: unexpected state %s\n",
                jtag_state_names[chain->jstate]);
        return -1;
    }

    for (i=(bytes+3)/4; i; i--) {
        ems_u32 d;
        for (j=32; j; j--) {
            if (jtag_action(chain, 0, 0, 0, j)) return -1;
        }
        if (jtag_data(chain, &d)) return -1;
        if (count++<10) printf("data= %08x\n", d);
        *data=d;
        data++;
    }
    return 0;
}
/****************************************************************************/
static int
jtag_wr_data32(struct jtag_chip* chip, int bits, ems_u32 data)
{
    struct jtag_chain* chain=chip->chain;
    int i;

    printf("jtag_wr_data32 %d 0x%x\n", bits, data);

    if (bits<1) {
        printf("jtag_wr_data32: bits=%d (illegal)\n", bits);
        return -1;
    }

    switch (chain->jstate) {
    case jtag_RTI:
    case jtag_UI:
        if (jtag_action(chain, 1, 0, 0, -1)) return -1; /* SDS */
        /* no break */
    case jtag_SDS:
        if (jtag_action(chain, 0, 0, 0, -1)) return -1; /* CD */
        if (jtag_action(chain, 0, 0, 0, -1)) return -1; /* SD */
        break;
    default:
        printf("jtag_wr_data32: unexpected state %s\n",
                jtag_state_names[chain->jstate]);
        return -1;
    }

    for (i=bits-1; i>=0; i--) {
        /* SD shift data */
        if (jtag_action(chain, i?0:!chip->after_d_bits, data&1, 0, i)) return -1;
        data>>=1;
    }

    for (i=chip->after_d_bits-1; i>=0; i--) {
        if (jtag_action(chain, i?0:1, 0, 0, i)) return -1; /* SD/E1D */
    }

    if (jtag_action(chain, 1, 0, 0, -1)) return -1;     /* UD */

    return 0;
}
/****************************************************************************/
/*
 * state before: RTI
 * state after:  UI
 * fills 'chip' with 'icode', all other chips with 'BYPASS'
 */
static int
jtag_instruction(struct jtag_chip* chip, ems_u32 icode)
{
    struct jtag_chain* chain=chip->chain;
    int i;

    switch (chain->jstate) {
    case jtag_TLR:
        if (jtag_action(chain, 0, 0, 0, -1)) return -1; /* RTI */
    case jtag_RTI:
    case jtag_UD:
        if (jtag_action(chain, 1, 0, 0, -1)) return -1; /* SDS */
        if (jtag_action(chain, 1, 0, 0, -1)) return -1; /* SIS */
        if (jtag_action(chain, 0, 0, 0, -1)) return -1; /* CI */
        if (jtag_action(chain, 0, 1, 0, -1)) return -1; /* SI */
        break;
    default:
        printf("jtag_instruction: unexpected state %s\n",
                jtag_state_names[chain->jstate]);
        return -1;
    }

    for (i=chip->pre_c_bits; i>0; i--) {
        if (jtag_action(chain, 0, 1, 0, i)) return -1;                 /* SI */
    }
    for (i=chip->chipdata->ir_len-1; i>=0; i--) {
        if (jtag_action(chain, i?0:!chip->after_c_bits, icode&1, 0, i))
            return -1; /* SI/E1I */
        icode>>=1;
    }
    for (i=chip->after_c_bits-1; i>=0; i--) {
        if (jtag_action(chain, i?0:1, 1, 0, i)) return -1;         /* SI/E1I */
    }

    if (jtag_action(chain, 1, 1, 0, -1)) return -1;                    /* UI */

    return 0;
}
/****************************************************************************/
/*
 * state before: RTI
 * state after:  UI
 * fills 'chip' with 'icode', all other chips with 'BYPASS'
 */
static int
jtag_instruction_r(struct jtag_chip* chip, ems_u32 icode, ems_u32* ret)
{
    struct jtag_chain* chain=chip->chain;
    ems_u32 out;
    int i;

printf("jtag_instruction_r icode=0x%x\n", icode);
    switch (chain->jstate) {
    case jtag_TLR:
        if (jtag_action(chain, 0, 0, 0, -1)<0) return -1; /* RTI */
    case jtag_RTI:
    case jtag_UD:
        if (jtag_action(chain, 1, 0, 0, -1)<0) return -1; /* SDS */
        if (jtag_action(chain, 1, 0, 0, -1)<0) return -1; /* SIS */
        if (jtag_action(chain, 0, 0, 0, -1)<0) return -1; /* CI */
        if (jtag_action(chain, 0, 1, 0, -1)<0) return -1; /* SI */
        break;
    default:
        printf("jtag_instruction: unexpected state %s\n",
                jtag_state_names[chain->jstate]);
        return -1;
    }

    out=0;
    for (i=chip->pre_c_bits; i>0; i--) {
        int x;
        if (jtag_action(chain, 0, 1, &x, i)<0) return -1;              /* SI */
        out>>=1;
        out|=x<<31;
        printf("Xout0=0x%08x\n", out);
    }
    for (i=chip->chipdata->ir_len-1; i>=0; i--) {
        int x;
        if (jtag_action(chain, i?0:!chip->after_c_bits, icode&1, &x, i)<0)
            return -1; /* SI/E1I */
        icode>>=1;
        out>>=1;
        out|=x<<31;
        printf("Xout1=0x%08x\n", out);
    }
    printf("jtag_instruction_r: out=0x%x\n", out);
    for (i=chip->after_c_bits-1; i>=0; i--) {
        int x;
        if (jtag_action(chain, i?0:1, 1, &x, i)<0) return -1;      /* SI/E1I */
        out>>=1;
        out|=x<<31;
        printf("Xout2=0x%08x\n", out);
    }

    if (jtag_action(chain, 1, 1, 0, -1)<0) return -1;                  /* UI */
    if (ret!=0)
        *ret=out;

    return 0;
}
/****************************************************************************/
static int
jtag_runtest(struct jtag_chain* chain, int count)
{
    switch (chain->jstate) {
    case jtag_TLR:
    case jtag_UD:
    case jtag_UI:
        jtag_action(chain, 0, 0, 0, -1); /* RTI */
        break;
    case jtag_RTI:
        /* do nothing */
        break;
    default:
        printf("jtag_instruction: unexpected state %s\n",
                jtag_state_names[chain->jstate]);
        return -1;
    }

    for (; count>=0; count--) {
        jtag_action(chain, 0, 0, 0, -1);
    }
    return 0;
}
/****************************************************************************/
/****************************************************************************/
plerrcode
jtag_init_dev(struct generic_dev* dev, struct jtag_dev* jdev, int ci, int addr)
{
    plerrcode pres;

    jdev->dev=dev;
    jdev->ci=ci;
    jdev->addr=addr;
    jdev->jt_data=0;
    jdev->jt_action=0;
    jdev->opaque=0;
    switch (dev->generic.class) {
#ifdef LOWLEVEL_UNIXVME
    case modul_vme: {
            struct vme_dev* vme_dev=(struct vme_dev*)dev;
            pres=vme_dev->get_jtag_procs(vme_dev, jdev);
        } break;
#endif
#ifdef LOWLEVEL_LVD
    case modul_lvd: {
            struct lvd_dev* lvd_dev=(struct lvd_dev*)dev;
            pres=lvd_dev->init_jtag_dev(lvd_dev, jdev);
        } break;
#endif
    default:
        printf("init_jtag_dev: jtag for this device (class %d) not supported\n",
                dev->generic.class);
        pres=plErr_BadModTyp;
    }
    return pres;
}
/****************************************************************************/
static int
jtag_read_ids(struct jtag_chain* chain, ems_u32** IDs, int* IDnum)
{
    ems_u32* ids=0;
    ems_u32 id;
    int i, num=0;

    *IDs=0;
    *IDnum=0;

printf("jtag_read_ids:\n");

    if (jtag_force_TLR(chain))
        return -1;

    jtag_action(chain, 0, 0, 0, -1);                                  /* RTI */
    jtag_action(chain, 1, 0, 0, -1);                                  /* SDS */
    jtag_action(chain, 0, 0, 0, -1);                                  /* CD */

    do {
        for (i=0; i<32; i++) {  /*  shift DR */
            jtag_action(chain, 0, 0, 0, i);
        }
        jtag_data(chain, &id);
        if (id) {
            printf("jtag ID 0x%08x\n", id);
            ids=realloc(ids, sizeof(ems_u32)*(num+1));
            if (ids==0) {
                printf("jtag_read_ids: malloc: %s\n", strerror(errno));
                return -1;
            }
            for (i=num; i>0; i--)
                ids[i]=ids[i-1];
            ids[0]=id;
            num++;
        }
    } while (id && (num<8));

    /* return to RTI state */
    jtag_action(chain, 1, 0, 0, -1);                                  /* E1D */
    jtag_action(chain, 1, 0, 0, -1);                                  /* UD */
    jtag_action(chain, 0, 0, 0, -1);                                  /* RTI */

    *IDs=ids;
    *IDnum=num;
    return 0;
}
/****************************************************************************/
void
jtag_free_chain(struct jtag_chain* chain)
{
    free(chain->chips);
    chain->chips=0;
}
/****************************************************************************/
plerrcode
jtag_init_chain(struct jtag_chain* chain)
{
    int i, bits;
    ems_u32* ids;

    if (jtag_read_ids(chain, &ids, &chain->num_chips))
        return plErr_System;

printf("jtag_init_chain:\n");

    chain->chips=malloc(sizeof(struct jtag_chip)*chain->num_chips);
    chain->valid=1;
    chain->state_count=0;

    for (i=0; i<chain->num_chips; i++) {
        struct jtag_chip* chip=chain->chips+i;
        chip->chain=chain;
        chip->id=ids[i];
        chip->version=(ids[i]>>28)&0xf;
        find_jtag_chipdata(chip);
        if (chip->chipdata==0)
            chain->valid=0;
    }

    if (!chain->valid) {
        free(ids);
        return plErr_BadModTyp;
    }

    for (i=0, bits=0; i<chain->num_chips; i++) {
        struct jtag_chip* chip=chain->chips+i;
        chip->pre_d_bits=chain->num_chips-i-1;
        chip->after_d_bits=i;
        chip->after_c_bits=bits;
        bits+=chip->chipdata->ir_len;
    }
    for (i=chain->num_chips-1, bits=0; i>=0; i--) {
        struct jtag_chip* chip=chain->chips+i;
        chip->pre_c_bits=bits;
        bits+=chip->chipdata->ir_len;
    }

    free(ids);
    return plOK;
}
/****************************************************************************/
void
jtag_dump_chain(struct jtag_chain* chain)
{
    int i;

    for (i=0; i<chain->num_chips; i++) {
        struct jtag_chip* chip=chain->chips+i;
        printf("jtag ID 0x%08x ", chip->id);
        if (chip->chipdata!=0) {
            printf("  %s ir_len=%d ver=%d\n", chip->chipdata->name,
                chip->chipdata->ir_len, chip->version);
            printf("pre_d=%d after_d=%d pre_c=%d after_c=%d ir=%d %s\n",
                chip->pre_d_bits, chip->after_d_bits,
                chip->pre_c_bits, chip->after_c_bits,
                chip->chipdata->ir_len,
                chip->chipdata->name);
        } else {
            printf("  chip not known\n");
        }
    }
}
/****************************************************************************/
int
jtag_irlen(struct jtag_chain* chain)
{
    int maxlen=(chain->num_chips+1)*32, n=0, i;

    if (jtag_force_TLR(chain))
        return -1;

    jtag_action(chain, 0, 0, 0, -1);                                  /* RTI */
    jtag_action(chain, 1, 0, 0, -1);                                  /* SDS */
    jtag_action(chain, 1, 0, 0, -1);                                  /* SIS */
    jtag_action(chain, 0, 0, 0, -1);                                  /* CI */
    for (i=0; i<maxlen; i++) {
        jtag_action(chain, 0, 0, 0, i);                               /* SI */
    }
    for (i=0; i<maxlen; i++) {
        int d;
        jtag_action(chain, 0, 1, &d, i);                              /* SI */
        if (!d)
            n=i;
    }
    /*printf("n1: %d\n", n+2);*/

    /* return to RTI state */
    /* !!! all instruction registers MUST contain BYPASS when we enter E1I */
    jtag_action(chain, 1, 0, 0, -1);                                  /* E1I */
    jtag_action(chain, 1, 0, 0, -1);                                  /* UI */
    jtag_action(chain, 0, 0, 0, -1);                                  /* RTI */

    return n+2;
}
/****************************************************************************/
int
jtag_datalen(struct jtag_chain* chain)
{
    int maxlen=(chain->num_chips+1)*32, n=0, i;

    if (jtag_force_TLR(chain))
        return -1;

    jtag_action(chain, 0, 0, 0, -1);                                  /* RTI */
    jtag_action(chain, 1, 0, 0, -1);                                  /* SDS */
    jtag_action(chain, 1, 0, 0, -1);                                  /* SIS */
    jtag_action(chain, 0, 0, 0, -1);                                  /* CI */
    /* set all chips to 'bypass' */
    for (i=0; i<maxlen; i++) {
        jtag_action(chain, 0, 1, 0, i);                               /* SI */
    }
    jtag_action(chain, 1, 0, 0, -1);                                  /* E1I */
    jtag_action(chain, 1, 0, 0, -1);                                  /* UI */
    jtag_action(chain, 1, 0, 0, -1);                                  /* SDS */
    jtag_action(chain, 0, 0, 0, -1);                                  /* CD */
    for (i=0; i<maxlen; i++) {
        jtag_action(chain, 0, 1, 0, i);                               /* SD */
    }
    for (i=0; i<maxlen; i++) {
        int d;
        jtag_action(chain, 0, 0, &d, i);                              /* SD */
        if (d)
            n=i;
    }
    printf("n1: %d\n", n+2);

    /* return to RTI state */
    jtag_action(chain, 1, 0, 0, -1);                                  /* E1D */
    jtag_action(chain, 1, 0, 0, -1);                                  /* UD */
    jtag_action(chain, 0, 0, 0, -1);                                  /* RTI */

    return n+2;
}
/****************************************************************************/
int
jtag_id(struct jtag_chip* chip, ems_u32* id)
{
    struct jtag_chain* chain=chip->chain;

    if (jtag_force_TLR(chain))
        return -1;

    if (jtag_instruction(chip, IDCODE))
        return -1;

    if (jtag_rd_data32(chip, 32, id))
        return -1;
    printf("ID=0x%08x\n", *id);

    return 0;
}
/*****************************************************************************/
int
jtag_read_XC18V00(struct jtag_chip* chip, void* data)
{
    struct jtag_chain* chain=chip->chain;
    ems_u32 id, ret;

    printf("reading device %s\n", chip->chipdata->name);

    if (jtag_id(chip, &id))
        return -1;

    if ((id&0x0fffffff)!=chip->id) {
        printf("jtag_read_XC18V00: wrong IDCODE: 0x%x\n", id);
        return -1;
    }

    if (jtag_force_TLR(chain))
        return -1;

    if (jtag_instruction_r(chip, ISPEN, &ret)) return -1;
    printf("ISPEN: ret=0x%x\n", ret);
    if (jtag_wr_data32(chip, 6, 0x34)) return -1;
    if (jtag_instruction_r(chip, FADDR, &ret)) return -1;
    printf("FADDR: ret=0x%x\n", ret);
    if (jtag_wr_data32(chip, 16, 0)) return -1;
    if (jtag_instruction_r(chip, FVFY1, &ret)) return -1;
    printf("FVFY1: ret=0x%x\n", ret);
    if (jtag_runtest(chain, 100)<0) return -1;
trace=0;
    if (jtag_rd_data(chip, chip->chipdata->mem_size, data)) {
        return -1;
    }
    if (jtag_goto_RTI(chain)) return -1;

    return 0;
}
/****************************************************************************/
/****************************************************************************/
