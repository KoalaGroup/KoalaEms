/*
 * lowlevel/lvd/tdc/f1.c
 * created 12.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: f1.c,v 1.17 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../commu/commu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <modultypes.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "f1.h"
#include "../lvd_access.h"
#include "../lvd_initfuncs.h"
#include "../../oscompat/oscompat.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/tdc")

/*****************************************************************************/
static int
lvd_start_f1(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct f1_info* info=(struct f1_info*)acard->cardinfo;
    int addr=acard->addr;
    int f1, res=0;
    int r1, r7;

    /* disable all inputs */
    res|=f1_reg_r(dev, acard, 0, 1, &r1);
    res|=f1_reg_w(dev, acard, 8, 1, r1|0x20);

    /* enable readout */
    res|=lvd_i_w(dev, addr, f1.cr, acard->daqmode);

    /* beini */
    res|=f1_reg_r(dev, acard, 0, 1, &r7);
    res|=f1_reg_w(dev, acard, 8, 1, r7&~0x8000);
    res|=f1_reg_w(dev, acard, 8, 1, r7|0x8000);

    /* clear error flags */
    res|=lvd_i_w(dev, addr, f1.sr, 1);
    for (f1=0; f1<8; f1++) {
        res|=lvd_i_w(dev, addr, f1.f1_state[f1], 0xf0);
    }
    res|=lvd_i_w(dev, addr, f1.hit_ovfl[0], 0xffff);

    /* enable edges */
    for (f1=0; f1<8; f1++) {
        int r;
        for (r=0; r<4; r++) {
            res|=f1_reg_w(dev, acard, f1, r+2, info->edges[f1][r]);
        }
    }
    /* enable inputs */
    res|=f1_reg_w(dev, acard, 8, 1, r1&~0x20);

    return res?-1:0;
}
/*****************************************************************************/
static int
lvd_stop_f1(struct lvd_dev* dev, struct lvd_acard* acard)
{
    int f1, res=0;
    int r1;

    /* disable all inputs */
    res|=f1_reg_r(dev, acard, 0, 1, &r1);
    res|=f1_reg_w(dev, acard, 8, 1, r1|0x20);
    /* disable all edges */
    for (f1=0; f1<8; f1++) {
        int r;
        for (r=0; r<4; r++) {
            f1_reg_w(dev, acard, f1, r+2, 0);
        }
    }

    /* disable readout */
    res|=lvd_i_w(dev, acard->addr, f1.cr, 0);

    return res?-1:0;
}
/*****************************************************************************/
static int
lvd_clear_f1(struct lvd_dev* dev, struct lvd_acard* acard)
{
    int addr=acard->addr;
    int need_beini=0;
    int f1, res=0;

    for (f1=0; f1<8; f1++) {
        ems_u32 stat;
        lvd_i_r(dev, addr, f1.f1_state[f1], &stat);
        if (stat) {
            res|=lvd_i_w(dev, addr, f1.f1_state[f1], stat);
            res|=lvd_i_r(dev, addr, f1.f1_state[f1], &stat);
            if (stat)
                need_beini++;
        }
    }
    if (need_beini) {
        int val;
        printf("lvd_clear_f1: beini required\n");
        res|=f1_reg_r(dev, acard, 0, 7, &val);
        res|=f1_reg_w(dev, acard, 8, 7, val&~0x8000);
        res|=f1_reg_w(dev, acard, 8, 7, val);
    }

    res|=lvd_i_w(dev, addr, f1.sr, 1);
    for (f1=0; f1<8; f1++) {
        res|=lvd_i_w(dev, addr, f1.f1_state[f1], 0xf0);
    }
    res|=lvd_i_w(dev, addr, f1.hit_ovfl[0], 0xffff);

    return res?-1:0;
}
/*****************************************************************************/
static int
f1_init_chip(struct lvd_dev* dev, struct lvd_acard* acard, int f1,
        int edges, int hsdiv)
/* f1 may be '8' for all F1s */
/* f1<0 calls f1_init_chip for all F1s separately */
{
    struct f1_info* info=(struct f1_info*)acard->cardinfo;
    int edgereg, letra, i;
    ems_u16 v;

#if 0
    printf("f1_init_chip card 0x%x F1 %d hsdiv=%d\n", acard->addr, f1, hsdiv);
#endif

    if (f1<0) {
        int res=0;
        for (i=0; i<8; i++) {
            res|=f1_init_chip(dev, acard, i, edges, hsdiv);
        }
        return res;
    }

    edges&=3;
    letra=(edges&3)==3;
    edgereg=(edges<<14)|(edges<<6);

    /* store requested edges for use during start of readout */
    for (i=0; i<4; i++) {
        if (f1==8) {
            int j;
            for (j=0; j<8; j++) {
                info->edges[j][i]=edgereg;
            }
        } else {
            info->edges[f1][i]=edgereg;
        }
    }

    /* but in reality disable all edges */
    for (i=2; i<=5; i++) {
        if (f1_reg_w(dev, acard, f1, i, 0)<0) return -1;
    }

    if (f1_reg_w(dev, acard, f1, 7, 0)<0) return -1;

    v=F1_R15_ROSTA|F1_R15_COM|F1_R15_8BIT;
    if (f1_reg_w(dev, acard, f1, 15, v)<0) return -1;

    tsleep(2);
    v=0x1800|(F1_REFCLKDIV<<8)|hsdiv;
    if (f1_reg_w(dev, acard, f1, 10, v)<0) return -1;
    tsleep(2);

    if (f1_reg_w(dev, acard, f1, 0, 0)<0) return -1;

    v=letra?F1_R1_LETRA:0;
    if (f1_reg_w(dev, acard, f1, 1, v)<0) return -1;

    if (f1_reg_w(dev, acard, f1, 7, F1_R7_BEINIT)<0) return -1;
    tsleep(2);

    return 0;
}
/*****************************************************************************/
/*
 * returns -1: error
 *          0: pll not locked
 *          1: pll locked
 */
static int
f1_check_chip(struct lvd_dev* dev, struct lvd_acard* acard, int f1, int quiet)
{
    int addr=acard->addr;
    ems_u32 val;

    if (lvd_i_r(dev, addr, f1.f1_state[f1], &val)<0)
            return -1;
    if (!quiet && (val & (F1_NOLOCK|F1_NOINIT))) {
        printf("f1_check: card 0x%x f1 %d sr=0x%04x:", addr, f1, val);
        if (val&F1_NOLOCK)
            printf(" NOLOCK");
        if (val&F1_NOINIT)
            printf(" NOINIT");
        printf("\n");
    }
#if 0
    if (val)
        printf("F1[%d]: state=0x%x; ERROR!!!\n", f1, val);
#endif
    return !val;
}
/*****************************************************************************/
/*
 * returns -1: error or pll not locked
 *          0: pll locked
 */
static int
f1_check_chips(struct lvd_dev* dev, struct lvd_acard* acard, int f1)
{
    int res=0;

    if (acard==0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
            if (dev->acards[card].mtype==ZEL_LVD_TDC_F1) {
                res|=f1_check_chips(dev, dev->acards+card, f1);
            }
        }
        return res;
    }

    if ((f1<0) || (f1>7)) {
        int fi;
        for (fi=0; fi<8; fi++) {
            res|=f1_check_chips(dev, acard, fi);
        }
        return res;
    }

    res=f1_check_chip(dev, acard, f1, 0)==1?0:-1; 
    return res;
}
/*****************************************************************************/
plerrcode
f1_init_(struct lvd_dev* dev, struct lvd_acard* acard, int chip,
        int edges, int daqmode)
{
    int addr=acard->addr;
    int hsdiv, res;

    if (!dev->hsdiv) {
        complain("f1_init: controller %d not initialised", addr>>4);
        return plErr_ArgRange;
    }

    switch (dev->contr_mode) {
    case contr_gpx:
        if (dev->hsdiv!=0x98) {
            complain("f1_init: unusable hsdiv of GPX controller 0x%x (0x%x), "
                    "0x98 is required.", addr>>4, dev->hsdiv);
            return plErr_ArgRange;
        }
        hsdiv=0xa2;
        break;
    case contr_f1:
        hsdiv=dev->hsdiv;
        break;
    default:
        complain("f1_init: controller is of unknown type or in wrong mode %d",
                dev->contr_mode);
        return plErr_BadModTyp;
    }

    if ((chip<0) || (chip>7)) {
        /* f1_reset_chips resets _ALL_ F1 of the bord */
        if (f1_reset_chips(dev, acard)<0)
            return plErr_System;
    }

    acard->daqmode=daqmode;

    res=f1_init_chip(dev, acard, chip<0?8:chip, edges, hsdiv);
    if (res)
        return plErr_System;

    res=lvd_i_w(dev, addr, f1.f1_range, dev->tdc_range);
    if (res)
        return plErr_System;

    tsleep(10);
    res=f1_check_chips(dev, acard, chip);
    if (res)
        return plErr_HW;

    if ((chip<0) || (chip>7))
        printf("f1_init: input card 0x%x initialized.\n", addr);
    else
        printf("f1_init: input card 0x%x f1 %d initialized.\n",
                addr, chip);
    acard->initialized=1;

    return plOK;
}
/*****************************************************************************/
int
f1_state(struct lvd_dev* dev, int card, int f1)
{
    int res=0, i;
    ems_u32 val;

    if (card<0) {
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_F1) {
                res|=f1_state(dev, i, f1);
            }
        }
        return res;
    }
    if (f1<0) {
        for (i=0; i<8; i++) {
            res|=f1_state(dev, card, i);
        }
        return res;
    }

    res=lvd_i_r(dev, dev->acards[card].addr, f1.f1_state[f1], &val);
    if (res)
        return res;
    printf("[%03x %d] 0x%04x", dev->acards[card].addr, f1, val);
    if (val&F1_WRBUSY) printf(" WRBUSY");
    if (val&F1_NOINIT) printf(" NOINIT");
    if (val&F1_NOLOCK) printf(" NOLOCK");
    if (val&F1_HOFL)   printf(" H_OFL");
    if (val&F1_OOFL)   printf(" O_OFL");
    if (val&F1_SEQERR) printf(" SEQERR");
    if (val&F1_IOFL)   printf(" I_OFL");
    printf("\n");
    return 0;
}
/*****************************************************************************/
int
f1_clear(struct lvd_dev* dev, int addr, int f1, ems_u32 val)
{

    if (addr<0) {
        int res=0, i;
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_F1) {
                res|=f1_clear(dev, dev->acards[i].addr, f1, val);
            }
        }
        return res;
    }
    if (f1<0) {
        int res=0, i;
        for (i=0; i<8; i++) {
            res|=f1_clear(dev, addr, i, val);
        }
        return res;
    }

     return lvd_i_w(dev, addr, f1.f1_state[f1], val);
}
/*****************************************************************************/
/*
 * if raw=0:
 * latency and width are given in ns
 */
plerrcode
f1_window_(struct lvd_dev* dev, struct lvd_acard* acard, int latency,
    int width, int raw)
{
    int res=0;

    printf("f1_window addr=0x%x latency=%d width=%d raw=%d\n",
            acard->addr, latency, width, raw);

    if (!raw) {
        latency=(latency*1000)/dev->tdc_resolution;
        width=(width*1000)/dev->tdc_resolution;
    }
    if ((latency&~0x3fff) || (width&~0x7fff))
        return plErr_ArgRange;

    res|=lvd_i_w(dev, acard->addr, f1.trg_lat, latency);
    res|=lvd_i_w(dev, acard->addr, f1.trg_win, width);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode
f1_getwindow_(struct lvd_dev* dev, struct lvd_acard* acard, ems_u32* latency,
    ems_u32* width, int raw)
{
    int res=0;

    res|=lvd_i_r(dev, acard->addr, f1.trg_lat, latency);
    res|=lvd_i_r(dev, acard->addr, f1.trg_win, width);
    if (!raw) {
        *latency=(*latency*dev->tdc_resolution)/1000;
        *width=(*width*dev->tdc_resolution)/1000;
    }
    return res?plErr_System:plOK;
}
/*****************************************************************************/
int
f1_reg(struct lvd_dev* dev, int addr, int f1, int reg, int val)
{
    struct lvd_acard* acard;
    int idx, res=0;
    DEFINE_LVDTRACE(trace);

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_F1) {
                res|=f1_reg(dev, dev->acards[i].addr, f1, reg, val);
            }
        }
        return res;
    }

    if (f1<0) {
        int i;
        for (i=0; i<8; i++) {
            res|=f1_reg(dev, addr, i, reg, val);
        }
        return res;
    }

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_F1)) {
        complain("f1_reg: no F1 card with address 0x%x known", addr);
        return plErr_Program;
    }

    lvd_settrace(dev, &trace, 1);
    res=f1_reg_w(dev, acard, f1, reg, val);
    lvd_settrace(dev, 0, trace);

    return res;
}
/*****************************************************************************/
int
f1_regbits(struct lvd_dev* dev, int addr, int f1, int reg, int val, int mask)
{
    struct lvd_acard* acard;
    int idx, res=0, oldval, newval;

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_F1) {
                res|=f1_regbits(dev, dev->acards[i].addr, f1, reg, val, mask);
            }
        }
        return res;
    }

    if (f1<0) {
        int i;
        for (i=0; i<8; i++) {
            res|=f1_regbits(dev, addr, i, reg, val, mask);
        }
        return res;
    }

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_F1)) {
        complain("f1_reg: no F1 card with address 0x%x known", addr);
        return plErr_Program;
    }

    f1_reg_r(dev, acard, f1, reg, &oldval);
    newval=(oldval&~mask)|(val&mask);
    res=f1_reg_w(dev, acard, f1, reg, newval);

    return res;
}
/*****************************************************************************/
int
f1_get_shadow(struct lvd_dev* dev, int addr, int f1, int reg, ems_u32** out)
{
    struct lvd_acard* acard;
    int idx, val;

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_F1) {
                f1_get_shadow(dev, dev->acards[i].addr, f1, reg, out);
            }
        }
        return 0;
    }

    if (f1<0) {
        int i;
        for (i=0; i<8; i++) {
            f1_get_shadow(dev, addr, i, reg, out);
        }
        return 0;
    }

    if (reg<0) {
        int i;
        for (i=0; i<16; i++) {
            f1_get_shadow(dev, addr, f1, i, out);
        }
        return 0;
    }

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_F1)) {
        printf("f1_get_shadow: no F1 card with address 0x%x known\n", addr);
        return -1;
    }

    f1_reg_r(dev, acard, f1, reg, &val);
    printf("f1_shadow 0x%03x/%d/%02d: 0x%04x\n",  addr, f1, reg, val);
    **out=val;
    (*out)++;

    return 0;
}
/*****************************************************************************/
plerrcode
f1_dac_(struct lvd_dev* dev, struct lvd_acard* acard, int connector, int dac,
        int version)
{
    dac&=0xff;
    if (connector<0) {
        int reg1;
        /* unconditionally set all (theortically existing) dacs */
        f1_reg_w(dev, acard, 8, 11, dac|dac<<8);
        f1_reg_w(dev, acard, 8, 12, dac|dac<<8);
        f1_reg_w(dev, acard, 8, 13, dac|dac<<8);
        f1_reg_w(dev, acard, 8, 14, dac|dac<<8);
        /* we assume that all F1 chips have the same setting of reg. 1 */
        f1_reg_r(dev, acard, 0, 1, &reg1);
        f1_reg_w(dev, acard, 8, 1, reg1|1); /* set DA bit */
        printf("f1_dac_[%03x]: all dacs set to %d\n", acard->addr, dac);
        return plOK;
    } else {
        int f1, reg, shift, regv, reg1;
        switch (version) {
        case 1: /* front connector (analog output) */ {
                f1=connector<<1;
                reg=11;
                shift=0;
            }
            break;
        case 2: /* rear connector (digital output) */ {
                f1=6;
                reg=(connector>>1)+11;
                shift=(connector&1)<<3;
            }
            break;
        default:
            printf("f1_dac_: invalid version %d\n", version);
            return plErr_Program;
        }
        f1_reg_r(dev, acard, f1, reg, &regv); /* comes from shadow */
        regv=(regv&(0xff00>>shift))|(dac<<shift);
        f1_reg_w(dev, acard, f1, reg, regv);
        f1_reg_r(dev, acard, f1, 1, &reg1);
        f1_reg_w(dev, acard, f1, 1, reg1|1); /* set DA bit */
        printf("f1_dac_[%03x]: F1 %d reg %d set to 0x%04x\n", acard->addr, f1, reg,
                regv);

        return plOK;
    }
}
/*****************************************************************************/
#if 0
plerrcode
f1_dac(struct lvd_dev* dev, int addr, int connector, int dac, int version)
{
    struct lvd_acard* acard;
    int idx;

    if (addr<0) {
        int card, res=0;
        for (card=0; card<dev->num_acards; card++) {
            if (dev->acards[card].mtype==ZEL_LVD_TDC_F1) {
                res|=f1_dac(dev, dev->acards[card].addr, connector, dac, version);
            }
        }
        return res?plErr_System:plOK;
    }

    /* find module and check type */
    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_F1)) {
        printf("f1_dac: no F1 card with address 0x%x known\n", addr);
        return plErr_Program;
    }

    dac&=0xff;
    if (connector<0) {
        int reg1;
        /* unconditionally set all (theortically existing) dacs */
        f1_reg_w(dev, acard, 8, 11, dac|dac<<8);
        f1_reg_w(dev, acard, 8, 12, dac|dac<<8);
        f1_reg_w(dev, acard, 8, 13, dac|dac<<8);
        f1_reg_w(dev, acard, 8, 14, dac|dac<<8);
        /* we assume that all F1 chips have the same setting of reg. 1 */
        f1_reg_r(dev, acard, 0, 1, &reg1);
        f1_reg_w(dev, acard, 8, 1, reg1|1); /* set DA bit */
        printf("f1_dac[%03x]: all dacs set to %d\n", addr, dac);
        return plOK;
    } else {
        int f1, reg, shift, regv, reg1;
        switch (version) {
        case 1: /* front connector (analog output) */ {
                f1=connector<<1;
                reg=11;
                shift=0;
            }
            break;
        case 2: /* rear connector (digital output) */ {
                f1=6;
                reg=(connector>>1)+11;
                shift=(connector&1)<<3;
            }
            break;
        default:
            printf("f1_dac: invalid version %d\n", version);
            return plErr_Program;
        }
        f1_reg_r(dev, acard, f1, reg, &regv); /* comes from shadow */
        regv=(regv&(0xff00>>shift))|(dac<<shift);
        f1_reg_w(dev, acard, f1, reg, regv);
        f1_reg_r(dev, acard, f1, 1, &reg1);
        f1_reg_w(dev, acard, f1, 1, reg1|1); /* set DA bit */
        printf("f1_dac[%03x]: F1 %d reg %d set to 0x%4x\n", addr, f1, reg, regv);

        return plOK;
    }
}
#endif
/*****************************************************************************/
static int
f1_test_hsdiv(struct lvd_dev* dev, struct lvd_acard* acard, int f1, int h)
{
    int res;

    res=f1_init_chip(dev, acard, f1, 0, h);
    if (res<0)
        return -1;

    tsleep(2);

    res=f1_check_chip(dev, acard, f1, 1);
    printf("test_hsdiv: addr=0x%x f1=%d h=%d: res=%d\n",
            acard->addr, f1, h, res);
    return res;
}
/*****************************************************************************/
static int
byte_wenden(int v)
{
    int i, w=0;
    for (i=0; i<8; i++) {
        w<<=1;
        w|=v&1;
        v>>=1;
    }
    return w;
}
/*****************************************************************************/
int
f1_hsdiv_search(struct lvd_dev* dev, int addr, int f1, int* hlow, int* hhigh)
{
    struct lvd_acard* acard;
    int h000, h255, hmiddle;
    int hu, hl, h, res;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_F1)) {
        printf("f1_hsdiv_search: no F1 card with address 0x%x known\n", addr);
        return plErr_Program;
    }

    /* find first 'locking' divider */
    h000=f1_test_hsdiv(dev, acard, f1, 0);
    h255=f1_test_hsdiv(dev, acard, f1, 255);
    if ((h000<0) || (h255<0)) {
        return -1;
    }
    hmiddle=-1;
    for (h=1; h<=255; h++) {
        int hw=byte_wenden(h);
        res=f1_test_hsdiv(dev, acard, f1, hw);
        if (res==1) {
            hmiddle=hw;
            break;
        }
    }
    if (hmiddle<0) {
        printf("%03x %d: no working hsdiv found\n", addr, f1);
        return -1;
    }
    /*printf("%03x %d 0:%d 255:%d %d:%d\n", addr, f1, h000, h255, hmiddle, 1);*/

    /* find lower threshold */
    if (h000==1) {
        *hlow=0;
    } else {
        hu=hmiddle;
        hl=0;
        res=0;
        while ((hu-hl)>1) {
            h=(hu+hl)/2;
            /*printf("hl=%d hu=%d h=%d\n", hl, hu, h);*/
            res=f1_test_hsdiv(dev, acard, f1, h);
            if (res==1)
                hu=h;
            else
                hl=h;
        }
        if (res==1)
            *hlow=h;
        else
            *hlow=hu;
        /*printf("res=%d hlow=%d\n", res, *hlow);*/
    }

    /* find upper threshold */
    if (h255==1) {
        *hhigh=255;
    } else {
        hl=hmiddle;
        hu=255;
        res=0;
        while ((hu-hl)>1) {
            h=(hu+hl)/2;
            /*printf("hl=%d hu=%d h=%d\n", hl, hu, h);*/
            res=f1_test_hsdiv(dev, acard, f1, h);
            if (res==1)
                hl=h;
            else
                hu=h;
        }
        if (res==1)
            *hhigh=h;
        else
            *hhigh=hl;
        /*printf("res=%d hlow=%d\n", res, *hlow);*/
    }
    /*printf("%03x %d: %d .. %d\n", addr, f1, *hlow, *hhigh);*/

    return 0;
}
/*****************************************************************************/
int
f1_hsdiv_search2(struct lvd_dev* dev, int addr, int f1, int* hlow, int* hhigh)
{
    struct lvd_acard* acard;
    const int hsdiv_start=0xa2;
    int idx, hsdiv, hsdiv_low, hsdiv_high;
    int locked;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_F1)) {
        printf("f1_hsdiv_search: no F1 card with address 0x%x known\n", addr);
        return plErr_Program;
    }

    /* find first working hsdiv, starting with hsdiv_start */
    /* search with increasing hsdiv */
    hsdiv=hsdiv_start;
    do {
        locked=f1_test_hsdiv(dev, acard, f1, hsdiv);
        if (!locked)
            hsdiv++;
    } while (!locked && hsdiv<=255);
    /* search with decreasing hsdiv if not locked yet */
    if (!locked) {
        hsdiv=hsdiv_start;
        do {
            locked=f1_test_hsdiv(dev, acard, f1, hsdiv);
            if (!locked)
                hsdiv--;
        } while (!locked && hsdiv>=0);
    }
    if (!locked) {
        printf("f1_hsdiv_search2 module 0x%x: no working hsdiv found\n", addr);
        return -1;
    }

    /* search first unusable hsdiv above initial hsdiv */
    hsdiv_high=hsdiv;
    do {
        locked=f1_test_hsdiv(dev, acard, f1, hsdiv_high);
        if (locked)
            hsdiv_high++;
    } while (locked && hsdiv_high<=255);
    if (!locked)
        hsdiv_high--;
    *hhigh=hsdiv_high;

    /* search first unusable hsdiv below initial hsdiv */
    hsdiv_low=hsdiv;
    do {
        locked=f1_test_hsdiv(dev, acard, f1, hsdiv_low);
        if (locked)
            hsdiv_low--;
    } while (locked && hsdiv_low>=0);
    if (!locked)
        hsdiv_low++;
    *hlow=hsdiv_low;

    return 0;
}
/*****************************************************************************/
plerrcode
f1_edge_(struct lvd_dev* dev, struct lvd_acard* acard, int channel,
        int edges, int immediate)
{
    struct f1_info* info=(struct f1_info*)acard->cardinfo;
    plerrcode pres=plOK;

    if (channel<0) { /* all 64 channels */
        int edgereg, f1, i;
        edgereg=(edges<<6)|(edges<<14);
        for (f1=0; f1<8; f1++) {        
            for (i=0; i<4; i++) {
                info->edges[f1][i]=edgereg;
            }
        }
        if (immediate) {
            for (i=2; i<=5; i++) {
                if (f1_reg_w(dev, acard, 8, i, edgereg)<0) {
                    pres=plErr_System;
                    break;;
                }
            }
        }
    } else { /* only 1 channel */
        int reg, shift, edgereg, f1;
        f1=channel>>3;
        channel&=7;       /* channel of f1 */
        reg=(channel>>1); /* f1 register-2 */
        channel&=1;       /* LSB of channel */
        shift=channel?14:6;
        edgereg=info->edges[f1][reg];
        edgereg&=~(3<<shift);
        edgereg|=edges<<shift;
        info->edges[f1][reg]=edgereg;
        if (immediate) {
            DEFINE_LVDTRACE(trace);
            lvd_settrace(dev, &trace, 1);
            if (f1_reg_w(dev, acard, f1, reg+2, edgereg)<0)
                pres=plErr_System;
            lvd_settrace(dev, 0, trace);
        }
    }

    return pres;
}
/*****************************************************************************/
plerrcode
f1_mask_(__attribute__((unused)) struct lvd_dev *dev, __attribute__((unused)) struct lvd_acard *acard, __attribute__((unused)) u_int32_t *mask)
{
    return plErr_NotImpl;
}
/*****************************************************************************/
plerrcode
f1_get_mask_(__attribute__((unused)) struct lvd_dev *dev, __attribute__((unused)) struct lvd_acard *acard, u_int32_t *mask)
{
    mask[0]=0;
    mask[1]=0;
    return plErr_NotImpl;
}
/*****************************************************************************/
/*****************************************************************************/
static int
dump_ident(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, f1.ident, &val)<0) {
        xprintf(xp, "  unable to read ident\n");
        return -1;
    }

    xprintf(xp, "  [%02x] ident    =0x%04x (type=0x%x version=0x%x)\n",
            0x00, val, val&0xff, (val>>8)&0xff);
    return 0;
}
/*****************************************************************************/
static int
dump_serial(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;
    int res;

    dev->silent_errors=1;
    res=lvd_i_r(dev, addr, f1.serial, &val);
    dev->silent_errors=0;
    if (res<0) {
        xprintf(xp, "  [%02x] serial   : not set\n", 0x02);
    } else {
        xprintf(xp, "  [%02x] serial   =%d\n", 0x02, val);
    }
    return 0;
}
/*****************************************************************************/
static int
dump_lat(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, f1.trg_lat, &val)<0) {
        xprintf(xp, "  unable to read trg_lat\n");
        return -1;
    }

    xprintf(xp, "  [%02x] trg_lat  =0x%04x\n", 0x04, val);
    return 0;
}
/*****************************************************************************/
static int
dump_win(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, f1.trg_win, &val)<0) {
        xprintf(xp, "  unable to read trg_win\n");
        return -1;
    }

    xprintf(xp, "  [%02x] trg_win  =0x%04x\n", 0x06, val);
    return 0;
}
/*****************************************************************************/
static int
dump_f1addr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, f1.f1_addr, &val)<0) {
        xprintf(xp, "  unable to read f1_addr\n");
        return -1;
    }

    xprintf(xp, "  [%02x] f1_addr  =0x%04x\n", 0x0a, val);
    return 0;
}
/*****************************************************************************/
static void decode_cr(ems_u32 cr, void *xp)
{
    if (cr&F1_CR_TMATCHING)
        xprintf(xp, " WINDOW");
    if (cr&F1_CR_RAW)
        xprintf(xp, " RAW");
    if (cr&F1_CR_LETRA)
        xprintf(xp, " LETRA");
    if (cr&F1_CR_EXTENDED)
        xprintf(xp, " EXTENDED");
}
/*****************************************************************************/
static int
dump_cr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 cr;

    if (lvd_i_r(dev, addr, f1.cr, &cr)<0) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }

    xprintf(xp, "  [%02x] cr       =0x%04x", 0x0c, cr);
    decode_cr(cr, xp);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_cr_saved(struct lvd_acard *acard, __attribute__((unused)) struct f1_info *info, void *xp)
{
    if (!acard->initialized)
        return 0;
    xprintf(xp, " saved cr        =0x%04x", acard->daqmode);
    decode_cr(acard->daqmode, xp);
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_sr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, f1.sr, &val)<0) {
        xprintf(xp, "  unable to read sr\n");
        return -1;
    }

    xprintf(xp, "  [%02x] sr       =0x%04x\n", 0x0e, val);
    return 0;
}
/*****************************************************************************/
static int
dump_f1state(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 stat[8];
    int f1, n=0;

    for (f1=0; f1<8; f1++) {
        lvd_i_r(dev, addr, f1.f1_state[f1], stat+f1);
        if (stat[f1])
            n++;
    }

    if (!n) {
        xprintf(xp, "  [%02x] f1_state[0..8]: all zero\n", 0x10);
    } else {
        for (f1=0; f1<8; f1++) {
            if (stat[f1]) {
                xprintf(xp, "  [%02x] F1_state[%d]: 0x%x", 0x10+2*f1, f1, stat[f1]);
                if (stat[f1]&1)
                    xprintf(xp, " WRBUSY");
                if (stat[f1]&2)
                    xprintf(xp, " NOINIT");
                if (stat[f1]&4)
                    xprintf(xp, " NOLOCK");
                if (stat[f1]&8)
                    xprintf(xp, " HANGUP (deadly)");
                if (stat[f1]&0x10)
                    xprintf(xp, " F1_HIT_OVFL");
                if (stat[f1]&0x20)
                    xprintf(xp, " F1_OUTPUT_OVFL");
                if (stat[f1]&0x40)
                    xprintf(xp, " SEQ_ERR");
                if (stat[f1]&0x80)
                    xprintf(xp, " INPUT_OVFL");
                xprintf(xp, "\n");
            }
        }
    }

    return 0;
}
/*****************************************************************************/
static int
dump_f1reg(struct lvd_dev* dev, struct lvd_acard* acard, void *xp)
{
    int val;
    int f1, reg;

    xprintf(xp, "  f1 registers (from shadow):\n");
    for (reg=0; reg<16; reg++) {
        xprintf(xp, "%4d ", reg);
    }
    xprintf(xp, "\n");
    for (f1=0; f1<8; f1++) {
        for (reg=0; reg<16; reg++) {
            f1_reg_r(dev, acard, f1, reg, &val);
            xprintf(xp, "%s%04x", reg?" ":"", val);
        }
        xprintf(xp, "\n");
    }

    return 0;
}
/*****************************************************************************/
static int
dump_f1range(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, f1.f1_range, &val)<0) {
        xprintf(xp, "  unable to read f1_range\n");
        return -1;
    }

    xprintf(xp, "  [%02x] f1_range =0x%04x\n", 0x4c, val);
    return 0;
}
/*****************************************************************************/
static int
dump_f1hitovfl(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;
    int i;

    xprintf(xp, "  [%02x] hit_ovfl[0..3]:", 0x50);
    for (i=0; i<4; i++) {
        if (lvd_i_r(dev, addr, f1.hit_ovfl[i], &val)<0) {
            xprintf(xp, "  unable to read hit_ovfl[%d]\n", i);
            return -1;
        }
        xprintf(xp, " 0x%04x", val);
    }
    xprintf(xp, "\n");

    return 0;
}
/*****************************************************************************/
static int
lvd_cardstat_f1(struct lvd_dev* dev, struct lvd_acard* acard, void *xp,
    int level)
{
    int addr=acard->addr;
    struct f1_info *info=(struct f1_info*)acard->cardinfo;

    xprintf(xp, "ACQcard 0x%03x F1\n", addr);

    switch (level) {
    case 0:
        dump_cr(dev, addr, xp);
        dump_sr(dev, addr, xp);
        dump_f1state(dev, addr, xp);
        dump_f1hitovfl(dev, addr, xp);
        break;
    case 1:
        dump_ident(dev, addr, xp);
        dump_lat(dev, addr, xp);
        dump_win(dev, addr, xp);
        dump_cr(dev, addr, xp);
        dump_cr_saved(acard, info, xp);
        dump_sr(dev, addr, xp);
        dump_f1state(dev, addr, xp);
        dump_f1range(dev, addr, xp);
        dump_f1hitovfl(dev, addr, xp);
        break;
    case 2:
    default:
        dump_ident(dev, addr, xp);
        dump_serial(dev, addr, xp);
        dump_lat(dev, addr, xp);
        dump_win(dev, addr, xp);
        dump_f1addr(dev, addr, xp);
        dump_cr(dev, addr, xp);
        dump_cr_saved(acard, info, xp);
        dump_sr(dev, addr, xp);
        dump_f1state(dev, addr, xp);
        dump_f1reg(dev, acard, xp);
        dump_f1range(dev, addr, xp);
        dump_f1hitovfl(dev, addr, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
static void
lvd_f1_acard_free(__attribute__((unused)) struct lvd_dev* dev, struct lvd_acard* acard)
{
    free(acard->cardinfo);
    acard->cardinfo=0;
}
/*****************************************************************************/
int
lvd_f1_acard_init(__attribute__((unused)) struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct f1_info *info;

    acard->freeinfo=lvd_f1_acard_free;
    acard->clear_card=lvd_clear_f1;
    acard->start_card=lvd_start_f1;
    acard->stop_card=lvd_stop_f1;
    acard->cardstat=lvd_cardstat_f1;
    acard->cardinfo=malloc(sizeof(struct f1_info));
    if (!acard->cardinfo) {
        printf("malloc acard->cardinfo(f1): %s\n", strerror(errno));
        return -1;
    }
    info=acard->cardinfo;

    memset(info->shadow, 0, sizeof(info->shadow));
    memset(info->edges, 0, sizeof(info->edges));
    info->last_f1=-1;

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
