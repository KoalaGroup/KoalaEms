/*
 * lowlevel/jtag/jtag_actions.c
 * created 2006-Jul-06 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_actions.c,v 1.4 2011/04/06 20:30:24 wuestner Exp $";

/*
 * This file contains the general jtag function but has now knowledge
 * of any particular chips.
 * It is only used by procedures of jtag_proc.c.
 */

#include <sconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <rcs_ids.h>

#include "jtag_int.h"

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

#ifdef JTAG_DEBUG
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
#endif

#define JT_UNKNOWN  -1             /* unknown device (corrupted JTAG chain) */
#define JT_RESET     0             /* state is RESET                        */
#define JT_IDLE      1             /* state is RTI (RUN-TEST/IDLE)          */
#define JT_DR_SHIFT  2             /* state is SHIFT-DR => jtag_rd_data()   */

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
#ifdef NEVER
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
#ifdef JTAG_DEBUG
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
#ifdef JTAG_DEBUG
    chain->oldjstate=chain->jstate;
#endif
    chain->jstate=jtag_transitions[chain->jstate][!!tms];
}
/****************************************************************************/
int
jtag_action(struct jtag_chain* chain, int tms, int tdi, int* tdo)
{
    struct jtag_dev* jtag_dev=&chain->jtag_dev;
    int tdo_, res;

    res=chain->jtag_dev.jt_action(jtag_dev, tms, tdi, &tdo_);
    if (tdo)
        *tdo=tdo_;
    changestate(chain, tms);

#ifdef JTAG_DEBUG
    if (chain->oldjstate==jtag_SI) {
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

    if (chain->oldjstate==jtag_SD) {
        int i;
        for (i=63; i>0; i--)
            chain->dchain[i]=chain->dchain[i-1];
        chain->dchain[0]=tdi?'1':'0';
        if (chain->jstate!=jtag_SD) {
            printf("DR: ");
            for (i=0; i<64; i++) {
                printf("%c", chain->dchain[i]);
            }
            printf("\n");
        }
    } else if (chain->jstate==jtag_SD) {
        int i;
        for (i=0; i<64; i++)
            chain->dchain[i]='_';
    }

    chain->ichain>>=1;
    chain->ochain>>=1;
    if (tdi)  chain->ichain|=0x80000000UL;
    if (tdo_) chain->ochain|=0x80000000UL;

    if (jtag_traced) {
        if (jtag_traced>1) {
            ems_u32 data;

            chain->jtag_dev.jt_data(jtag_dev, &data);

            printf("%16s tms %d tdi %d %16s tdo %d loop %d\n",
                jtag_state_names[chain->oldjstate],
                tms, tdi,
                jtag_state_names[chain->jstate],
                tdo_,
                chain->loopcount);

            printbits32(chain->ichain, "ichain ", " ");
            printf("%08x\n", chain->ichain);
            printbits32(chain->ochain, "ochain ", " ");
            printf("%08x\n", chain->ochain);
            printbits32(data  , "data   ", " ");
            printf("%08x\n", data);
        }

        if (chain->jstate!=chain->oldjstate) {
            printf("(%2d) %16s --> %16s\n", chain->jstate_count,
                    jtag_state_names[chain->oldjstate],
                    jtag_state_names[chain->jstate]);
            chain->jstate_count=1;
        } else {
            chain->jstate_count++;
        }
    }
#endif

    return res;
}
/****************************************************************************/
int
jtag_force_TLR(struct jtag_chain* chain)
{
    int i;

#ifdef JTAG_DEBUG
    printf("force_TLR\n");
#endif
    chain->jstate=jtag_TLR; /* not known yet, but forced by the following loop*/
    for (i=0; i<5; i++) {                                /* force TLR state */
        jtag_loopcount(chain, i);
        if (jtag_action(chain, 1, 0, 0)) {
            jtag_loopcount(chain, 0);
            return -1;
        }
    }
    jtag_loopcount(chain, 0);
    return 0;
}
/****************************************************************************/
int
jtag_read_ids(struct jtag_chain* chain, ems_u32** IDs, int* IDnum)
{
    ems_u32* ids=0;
    ems_u32 id;
    int i, num=0;

    *IDs=0;
    *IDnum=0;

    if (jtag_force_TLR(chain))
        return -1;

    jtag_action(chain, 0, 0, 0);                                  /* RTI */
    jtag_action(chain, 1, 0, 0);                                  /* SDS */
    jtag_action(chain, 0, 0, 0);                                  /* CD */

    do {
        for (i=0; i<32; i++) {  /*  shift DR */
            jtag_loopcount(chain, i);
            jtag_action(chain, 0, 0, 0);
        }
        jtag_loopcount(chain, 0);
        jtag_data(chain, &id);
        if (id) {
#ifdef JTAG_DEBUG
            printf("jtag ID 0x%08x\n", id);
#endif
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
    jtag_action(chain, 1, 0, 0);                                  /* E1D */
    jtag_action(chain, 1, 0, 0);                                  /* UD */
    jtag_action(chain, 0, 0, 0);                                  /* RTI */

    *IDs=ids;
    *IDnum=num;
    return 0;
}
/****************************************************************************/
int
jtag_irlen(struct jtag_chain* chain)
{
    int maxlen=(chain->num_chips+1)*32, n=0, i;

    if (jtag_force_TLR(chain))
        return -1;

    jtag_action(chain, 0, 0, 0);                                  /* RTI */
    jtag_action(chain, 1, 0, 0);                                  /* SDS */
    jtag_action(chain, 1, 0, 0);                                  /* SIS */
    jtag_action(chain, 0, 0, 0);                                  /* CI */

    for (i=0; i<maxlen; i++) {
        jtag_loopcount(chain, i);
        jtag_action(chain, 0, 0, 0);                               /* SI */
    }
    jtag_loopcount(chain, 0);

    for (i=0; i<maxlen; i++) {
        int d;
        jtag_loopcount(chain, i);
        jtag_action(chain, 0, 1, &d);                              /* SI */
        if (!d)
            n=i;
    }
    jtag_loopcount(chain, 0);

    /* return to RTI state */
    /* !!! all instruction registers MUST contain BYPASS when we enter E1I */
    jtag_action(chain, 1, 0, 0);                                  /* E1I */
    jtag_action(chain, 1, 0, 0);                                  /* UI */
    jtag_action(chain, 0, 0, 0);                                  /* RTI */

    return n+2;
}
/****************************************************************************/
int
jtag_datalen(struct jtag_chain* chain)
{
    int maxlen=(chain->num_chips+1)*32, n=0, i;

    if (jtag_force_TLR(chain))
        return -1;

    jtag_action(chain, 0, 0, 0);                                  /* RTI */
    jtag_action(chain, 1, 0, 0);                                  /* SDS */
    jtag_action(chain, 1, 0, 0);                                  /* SIS */
    jtag_action(chain, 0, 0, 0);                                  /* CI */

    /* set all chips to 'bypass' */
    for (i=0; i<maxlen; i++) {
        jtag_loopcount(chain, i);
        jtag_action(chain, 0, 1, 0);                               /* SI */
    }
    jtag_loopcount(chain, 0);

    jtag_action(chain, 1, 0, 0);                                  /* E1I */
    jtag_action(chain, 1, 0, 0);                                  /* UI */
    jtag_action(chain, 1, 0, 0);                                  /* SDS */
    jtag_action(chain, 0, 0, 0);                                  /* CD */

    for (i=0; i<maxlen; i++) {
        jtag_loopcount(chain, i);
        jtag_action(chain, 0, 1, 0);                               /* SD */
    }
    jtag_loopcount(chain, 0);

    for (i=0; i<maxlen; i++) {
        int d;
        jtag_loopcount(chain, i);
        jtag_action(chain, 0, 0, &d);                              /* SD */
        if (d)
            n=i;
    }
    jtag_loopcount(chain, 0);

    /* return to RTI state */
    jtag_action(chain, 1, 0, 0);                                  /* E1D */
    jtag_action(chain, 1, 0, 0);                                  /* UD */
    jtag_action(chain, 0, 0, 0);                                  /* RTI */

    return n+2;
}
/****************************************************************************/
int
jtag_instruction(struct jtag_chip* chip, ems_u32 icode, ems_u32* ret)
{
    struct jtag_chain* chain=chip->chain;
    int i;

    /* printf("instruction(icode=%x state=%s)\n",
        icode, jtag_state_names[chain->jstate]); */
    jtag_action(chain, 1, 1, 0); /* DR Select */
    jtag_action(chain, 1, 1, 0); /* IR Select */
    jtag_action(chain, 0, 1, 0); /* IR Capture */
    jtag_action(chain, 0, 1, 0); /* IR Shift */

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
    jtag_action(chain, 0, 1, 0); /* RTI */

    return 0;
}
/****************************************************************************/
int
jtag_data(struct jtag_chain* chain, ems_u32* d)
{
    struct jtag_dev* jdev=&chain->jtag_dev;
    return jdev->jt_data(jdev, d);
}
/****************************************************************************/
int
jtag_data32(struct jtag_chip* chip, ems_u32 din, ems_u32* dout, int len)
{
    struct jtag_chain* chain=chip->chain;
    int i;

    jtag_action(chain, 1, 1, 0);
    jtag_action(chain, 0, 1, 0);
    jtag_action(chain, 0, 1, 0);			/* SHIFT-DR */

    /* shift pre_bits */
    for (i=chip->pre_d_bits; i>0; i--) {
        jtag_action(chain, 0, 1, 0);
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

    jtag_action(chain, 1, 1, 0);        /* UD */
    jtag_action(chain, 0, 1, 0);	/* state RTI */

    return 0;
}
/****************************************************************************/
int
jtag_rd_data(struct jtag_chip* chip, ems_u32 *buf, int len)
{
    struct jtag_chain* chain=chip->chain;
    int i, j, k;

    if (len == 0) {
        if (chain->state == 1) {
            jtag_action(chain, 1, 1, 0);
            jtag_action(chain, 1, 1, 0);
            jtag_action(chain, 0, 1, 0);
            chain->state=0;                          /* state RTI */
        }
        return 0;
    }

    if (chain->state == 0) {
        jtag_action(chain, 1, 1, 0);
        jtag_action(chain, 0, 1, 0);         /* CAPTURE-DR */

        for (i=chip->pre_d_bits; i>0; i--) {
            jtag_action(chain, 0, 1, 0);
        }
        chain->state=1;
    }

    for (i=(len+3)/4, k=0; i; i--, k++) {
        ems_u32 d;
        for (j=32; j; j--)
            jtag_action(chain, 0, 1, 0);
        jtag_data(chain, &d);
        *buf++ =d;
        if (k<10)
            printf("%08x\n", d);
    }
    return 0;
}
/****************************************************************************/
int
jtag_wr_data(struct jtag_chip* chip, void *buf, int len, int end)
{
    struct jtag_chain* chain=chip->chain;
    ems_u32 *bf, din;
    ems_u8 ms, ms1;
    int i;

    bf=(ems_u32*)buf;
    len >>=2;
    if (len ==0) return 0;

    jtag_action(chain, 1, 1, 0); /* dr-select */
    jtag_action(chain, 0, 1, 0); /* capture-dr */
    jtag_action(chain, 0, 1, 0); /* shift-dr */

    for (i=chip->pre_d_bits; i>0; i--) {
        jtag_action(chain, 0, 1, 0); /* shift-dr */
    }

    ms=0;
    ms1=chip->after_d_bits?0:1;
    while (len) {
        din=*bf++;
        for (i=31; i>=0; i--) {
            if ((len==1) && !i) /* last bit, if no after_d_bits */
                ms=ms1;
            jtag_action(chain, ms, din&1, 0); /* shift-dr, exit-dr */
            din>>=1;
        }
        len--;
    }

    for (i=chip->after_d_bits-1; i>=0; i--) {
        if (!i) ms=1;
        jtag_action(chain, ms, 1, 0); /* shift-dr, exit-dr */
    }

    if (end < 0) {
        jtag_action(chain, 1, 1, 0); /* update-dr */
        jtag_action(chain, 0, 1, 0); /* idle */
        chain->state=0;
    }

    return 0;
}
/****************************************************************************/
/****************************************************************************/
