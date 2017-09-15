/*
 * lowlevel/lvd/tdc/gpx.c
 * created 2006-Feb-07 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: gpx.c,v 1.16 2013/01/17 22:44:54 wuestner Exp $";

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
#include "gpx.h"
#include "../lvd_access.h"
#include "../lvd_initfuncs.h"
#include "../../oscompat/oscompat.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct gpx_info {
    int edges[8];          /* [gpx] */
    int noedges;           /* edge register with no edges enabled */
    int dcm_shift[2];
};

RCS_REGISTER(cvsid, "lowlevel/lvd/tdc")

/*****************************************************************************/
static int
lvd_start_gpx(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct gpx_info* info=(struct gpx_info*)acard->cardinfo;
    int addr=acard->addr;
    int gpx, res=0;

    /*printf("write daqmode 0x%x to GPX card 0x%x\n",
            info->daqmode, addr);*/
    res|=lvd_i_w(dev, addr, gpx.cr, acard->daqmode);
    for (gpx=0; gpx<8; gpx++) {
        res|=lvd_i_w(dev, addr, gpx.gpx_seladdr, (gpx<<4)|0);
        res|=lvd_i_w(dev, addr, gpx.gpx_data, info->edges[gpx]);
    }

    return res?-1:0;
}
/*****************************************************************************/
static int
lvd_stop_gpx(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct gpx_info* info=(struct gpx_info*)acard->cardinfo;
    int addr=acard->addr;
    int res=0;

    /* disable all edges */
    res|=lvd_i_w(dev, addr, gpx.gpx_seladdr, (8<<4)|0);
    res|=lvd_i_w(dev, addr, gpx.gpx_data, info->noedges);
    res|=lvd_i_w(dev, addr, gpx.cr, 0);

    return res?-1:0;
}
/*****************************************************************************/
static int
lvd_clear_gpx(struct lvd_dev* dev, struct lvd_acard* acard)
{
    return 0;
}
/*****************************************************************************/
static int
gpx_reg_w(struct lvd_dev* dev, struct lvd_acard* acard, unsigned int gpx,
    int unsigned reg, int val)
{
    if (lvd_i_w(dev, acard->addr, gpx.gpx_seladdr, (gpx<<4)|reg)<0)
        return -1;
    if (lvd_i_w(dev, acard->addr, gpx.gpx_data, val)<0)
        return -1;
    return 0;
}
/*****************************************************************************/
static int
gpx_reg_r(struct lvd_dev* dev, struct lvd_acard* acard, unsigned int gpx,
    int unsigned reg, ems_u32* val)
{
    if (lvd_i_w(dev, acard->addr, gpx.gpx_seladdr, (gpx<<4)|reg)<0)
        return -1;
    if (lvd_i_r(dev, acard->addr, gpx.gpx_data, val)<0)
        return -1;
    return 0;
}
/*****************************************************************************/
/*
 * dcm: 0: setup time
 *      1: read time
 * abs: 0: relative
 *      1: absolute (uses shadow register)
 * val: relative or absolute shift
 */
static plerrcode
gpx_dcm_shift_(struct lvd_dev* dev, struct lvd_acard* acard, int dcm, int absol,
    int val)
{
    struct gpx_info* info=(struct gpx_info*)acard->cardinfo;
    int v, i, t;
    ems_u32 r;

    if (absol)
        val-=info->dcm_shift[dcm?1:0];
    if (val==0)
        return plOK;

    info->dcm_shift[dcm?1:0]+=val;

    v=1;
    t=1;
    if (val>0)
        v|=2;
    if (dcm) {
        v<<=4;
        t<<=4;
    }

    val=abs(val);
    /*printf("gpx_dcm_shift: will %d times write 0x%04x\n", val, v);*/
    for (i=0; i<val; i++) {
        if (lvd_i_w(dev, acard->addr, gpx.dcm_shift, v)<0)
            return plErr_System;
        do {
            if (lvd_i_r(dev, acard->addr, gpx.dcm_shift, &r)<0)
                return plErr_System;
        } while (!(r&t));
    }
    lvd_i_r(dev, acard->addr, gpx.dcm_shift, &r);
    if (r!=(t|0x44)) {
        complain("gpx_dcm_shift: status=0x%02x", r);
        return plErr_HW;
    }
    return plOK;
}
/*****************************************************************************/
/*
 * dcm: 0: setup time
 *      1: read time
 * abs: 0: relative
 *      1: absolute (uses shadow register)
 * val: relative or absolute shift
 */
plerrcode
gpx_dcm_shift(struct lvd_dev* dev, int addr, int dcm, int abs, int val)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_GPX) {
                pres=gpx_dcm_shift_(dev, dev->acards+i, dcm, abs, val);
                if (pres!=plOK)
                    return pres;
            }
        }
    } else {
        struct lvd_acard* acard;
        int idx;
        idx=lvd_find_acard(dev, addr);
        acard=dev->acards+idx;
        if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_GPX)) {
            complain("gpx_dcm_shift: no gpx card with address 0x%x known",
                    addr);
            return plErr_ArgRange;
        }
        pres=gpx_dcm_shift_(dev, acard, dcm, abs, val);
    }
    return pres;
}
/*****************************************************************************/
/*
 * returns -1: error
 *          0: pll not locked
 *          1: pll locked
 */
static int
gpx_check_chip(struct lvd_dev* dev, struct lvd_acard* acard, int gpx, int quiet)
{
    int count=0;
    ems_u32 val;
    DEFINE_LVDTRACE(trace);

    if (gpx_reg_r(dev, acard, gpx, 12, &val)<0)
                return -1;
    lvd_settrace(dev, &trace, 0);
    do {
        if (gpx_reg_r(dev, acard, gpx, 12, &val)<0)
                return -1;
    } while ((++count<10000) && (val&0x400));
    lvd_settrace(dev, 0, trace);

    if (val&0x400) {
        if (!quiet)
            printf("GPX %d of card 0x%x: no PLL lock!\n", gpx, acard->addr);
        return 0;
    } else {
        return 1;
    }
}
/*****************************************************************************/
#if 0
static int
gpx_check_chip(struct lvd_dev* dev, struct lvd_acard* acard, int gpx, int quiet)
{
    int res;

    res=_gpx_check_chip(dev, acard, gpx, quiet);
    if (res==0) {
        sleep(1);
        res=_gpx_check_chip(dev, acard, gpx, quiet);
    }
    return res;
}
#endif
/*****************************************************************************/
/*
 * returns -1: error or pll not locked
 *          0: pll locked
 */
static int
gpx_check_chips(struct lvd_dev* dev, struct lvd_acard* acard, int gpx)
{
    int res=0;

    if (acard==0) {
        int card;
        for (card=0; card<dev->num_acards; card++) {
            if (dev->acards[card].mtype==ZEL_LVD_TDC_GPX) {
                res|=gpx_check_chips(dev, dev->acards+card, gpx);
            }
        }
        return res;
    }

    if ((gpx<0) || (gpx>7)) {
        int fi;
        for (fi=0; fi<8; fi++) {
            res|=gpx_check_chips(dev, acard, fi);
        }
        return res;
    }

    res=gpx_check_chip(dev, acard, gpx, 0)==1?0:-1; 
    return res;
}
/*****************************************************************************/
static int
gpx_init_chip(struct lvd_dev* dev, struct lvd_acard* acard, int gpx,
        int edges, int hsdiv)
/* gpx may be '8' for all gpxs */
/* gpx<0 calls gpx_init_chip for all gpxs separately */
{
    struct gpx_info* info=(struct gpx_info*)acard->cardinfo;
    int edgereg, i;
    /*ems_u32 val;*/

#if 0
    printf("gpx_init_chip card 0x%x gpx %d hsdiv=%d\n", acard->addr, gpx,
            hsdiv);
#endif

    if (gpx<0) {
        int res=0;
        for (i=0; i<8; i++) {
            res|=gpx_init_chip(dev, acard, i, edges, hsdiv);
        }
        return res;
    }

#if 0
    /* test read */
    if (gpx_reg_r(dev, acard, gpx, 0, &val)<0) return -1;
    printf("gpx test read: 0x%x\n", val);
#endif

    /* set I-mode*/
    if (gpx_reg_w(dev, acard, gpx, 2, 0x2)<0) return -1;

    edges&=3;
    edgereg=0x481; /* ROsc, service bits and leading edge for start */
    /* disable all edges */
    if (gpx_reg_w(dev, acard, gpx, 0, edgereg)<0) return -1;

    /* store requested edges for use during start of readout */
    info->noedges=edgereg;
    if (edges&1)
        edgereg|=0x007f800;
    if (edges&2)
        edgereg|=0xff00000;
    if (gpx==8) {
        int i;
        for (i=0; i<8; i++) {
            info->edges[i]=edgereg;
        }
    } else {
        info->edges[gpx]=edgereg;
    }

    /* start PLL */
    if (hsdiv>=0) {
        if (gpx_reg_w(dev, acard, gpx, 7, 0x0001800|(GPX_REFCLKDIV<<8)|hsdiv)<0)
            return -1;
    } else {
        if (gpx_reg_w(dev, acard, gpx, 7, 0x0001f98)<0) /* 97.47 ps */
            return -1;
    }

    /* EFlag */
    if (gpx_reg_w(dev, acard, gpx, 4, 0x2000001)<0) return -1;

    /* trigger, retrigger */
    /*reg5=config.g_startoff1|(f1_sync<2?0x2000000:0xA000000);*/
    /*config.g_startoff1=0x20000*/
    if (gpx_reg_w(dev, acard, gpx, 5, 0xa020000)<0) return -1;

#if 0
    /* enable error flags */
    if (gpx_reg_w(dev, acard, gpx, 11, 0x7ff0000)<0) return -1;
#else
    /* enable PLL error, disable FIFO error flags */
    if (gpx_reg_w(dev, acard, gpx, 11, 0x4000000)<0) return -1;
#endif

#if 1
    /* set FIFO flags (???) */
    if (gpx_reg_w(dev, acard, gpx, 12, 0x4000000)<0) return -1;
#endif

    /* master reset */
    if (gpx_reg_w(dev, acard, gpx, 4, 0x2400001)<0) return -1;

    return 0;
}
/*****************************************************************************/
plerrcode
gpx_init_(struct lvd_dev* dev, struct lvd_acard* acard, int chip,
        int edges, int daqmode)
{
    int addr=acard->addr;
    int hsdiv, range, res;

    if (!dev->hsdiv) {
        complain("gpx_init: controller %d not initialised", addr>>4);
        return plErr_ArgRange;
    }

    switch (dev->contr_mode) {
    case contr_f1:
        if (LVD_FWver(acard->id)>=0x20) {
            complain("gpx_init: firmware too new to be used with F1 controller");
            return plErr_BadModTyp;
        }
        if (dev->hsdiv!=0xa2) {
            complain("gpx_init: unusable hsdiv of F1 controller 0x%x (0x%x), "
                    "0xa2 is required.", addr>>4, dev->hsdiv);
            return plErr_ArgRange;
        }
        hsdiv=0x98;
        range=dev->tdc_range;
        break;
    case contr_gpx:
        if (LVD_FWver(acard->id)<0x20) { /* F1 compatibility mode */
            complain("gpx_init: GPX controller in native mode is not usable");
            return plErr_BadModTyp;
        }
        range=dev->tdc_range;
        hsdiv=dev->hsdiv;
        break;
    default:
        complain("gpx_init: controller is of unknown type or in wrong mode %d",
                dev->contr_mode);
        return plErr_BadModTyp;
    }

    if ((chip<0) || (chip>7)) {
        /* this resets _ALL_ gpx of the bord */
        if (lvd_i_w(dev, acard->addr, gpx.ctrl, LVD_PURES)<0)
            return plErr_System;
    }

    acard->daqmode=daqmode;

    if (gpx_init_chip(dev, acard, chip<0?8:chip, edges, hsdiv)<0)
        return plErr_System;

    if (lvd_i_w(dev, addr, gpx.gpx_range, range)<0)
        return plErr_System;   

    if (LVD_FWver(acard->id)==0x11) {
        gpx_dcm_shift_(dev, acard, 0, 1, 20);
    }

    tsleep(10);
    res=gpx_check_chips(dev, acard, chip);
    if (res)
        return plErr_HW;

    if ((chip<0) || (chip>7))
        printf("gpx_init: input card 0x%x initialized.\n", addr);
    else
        printf("gpx_init: input card 0x%x gpx %d initialized.\n",
                addr, chip);
    acard->initialized=1;

    return plOK;
}
/*****************************************************************************/
/*
 * if raw=0:
 * latency and width are given in ns
 */
plerrcode
gpx_window_(struct lvd_dev* dev, struct lvd_acard* acard, int latency,
    int width, int raw)
{
    int res=0;

    printf("gpx_window addr=0x%x latency=%d width=%d raw=%d\n",
            acard->addr, latency, width, raw);

    if (!raw) {
        latency=(latency*1000)/dev->tdc_resolution;
        width=(width*1000)/dev->tdc_resolution;
    }
printf("latency=0x%x, width=0x%x\n", latency, width);
    if ((latency<-0x8000) || (latency>0x7fff))
        return plErr_ArgRange;
    if ((width<0) || (width>0xffff))
        return plErr_ArgRange;

    res|=lvd_i_w(dev, acard->addr, gpx.trg_lat, latency);
    res|=lvd_i_w(dev, acard->addr, gpx.trg_win, width);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode
gpx_getwindow_(struct lvd_dev* dev, struct lvd_acard* acard, ems_u32* latency,
    ems_u32* width, int raw)
{
    int res=0;

    res|=lvd_i_r(dev, acard->addr, gpx.trg_lat, latency);
    res|=lvd_i_r(dev, acard->addr, gpx.trg_win, width);
    if (!raw) {
        *latency=(*latency*dev->tdc_resolution)/1000;
        *width=(*width*dev->tdc_resolution)/1000;
    }
    return res?plErr_System:plOK;
}
/*****************************************************************************/
int
gpx_write_reg(struct lvd_dev* dev, int addr, int gpx, int reg, int val)
{
    struct lvd_acard* acard;
    int idx, res=0;
    DEFINE_LVDTRACE(trace);

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_GPX) {
                res|=gpx_write_reg(dev, dev->acards[i].addr, gpx, reg, val);
            }
        }
        return res;
    }

    if (gpx<0) {
        int i;
        for (i=0; i<8; i++) {
            res|=gpx_write_reg(dev, addr, i, reg, val);
        }
        return res;
    }

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_GPX)) {
        complain("gpx_reg: no gpx card with address 0x%x known", addr);
        return plErr_Program;
    }

    lvd_settrace(dev, &trace, 1);
    res=gpx_reg_w(dev, acard, gpx, reg, val);
    lvd_settrace(dev, 0, trace);

    return res;
}
/*****************************************************************************/
int
gpx_read_reg(struct lvd_dev* dev, int addr, int gpx, int reg, ems_u32* val)
{
    struct lvd_acard* acard;
    int idx, res=0;
    DEFINE_LVDTRACE(trace);

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_GPX)) {
        complain("gpx_reg: no gpx card with address 0x%x known", addr);
        return plErr_Program;
    }

    lvd_settrace(dev, &trace, 1);
    res=gpx_reg_r(dev, acard, gpx, reg, val);
    lvd_settrace(dev, 0, trace);

    return res;
}
/*****************************************************************************/
static int
gpx_test_hsdiv(struct lvd_dev* dev, struct lvd_acard* acard, int chip, int h)
{
    int res;

    /*printf("test_hsdiv: addr=0x%x f1=%d h=%d\n", addr, f1, h);*/
    res=gpx_init_chip(dev, acard, chip, 0, h);
    if (res<0)
        return -1;

    tsleep(2);

    res=gpx_check_chip(dev, acard, chip, 1);
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
gpx_hsdiv_search(struct lvd_dev* dev, int addr, int chip, int* hlow, int* hhigh)
{
    struct lvd_acard* acard;
    int h000, h255, hmiddle;
    int hu, hl, h, res;
    int idx;

    idx=lvd_find_acard(dev, addr);
    acard=dev->acards+idx;
    if ((idx<0) || (acard->mtype!=ZEL_LVD_TDC_GPX)) {
        printf("gpx_hsdiv_search: no GPX card with address 0x%x known\n", addr);
        return -1;
    }

    /* find first 'locking' divider */
    h000=gpx_test_hsdiv(dev, acard, chip, 0);
    h255=gpx_test_hsdiv(dev, acard, chip, 255);
    if ((h000<0) || (h255<0)) {
        return -1;
    }
    hmiddle=-1;
    for (h=1; h<=255; h++) {
        int hw=byte_wenden(h);
        res=gpx_test_hsdiv(dev, acard, chip, hw);
        if (res==1) {
            hmiddle=hw;
            break;
        }
    }
    if (hmiddle<0) {
        printf("%03x %d: no working hsdiv found\n", addr, chip);
        return -1;
    }
#if 1
    printf("%03x %d 0:%d 255:%d %d\n", addr, chip, h000, h255, hmiddle);
#endif

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
            res=gpx_test_hsdiv(dev, acard, chip, h);
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
            res=gpx_test_hsdiv(dev, acard, chip, h);
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
#if 0
    printf("%03x %d: %d .. %d\n", addr, chip, *hlow, *hhigh);
#endif
    return 0;
}
/*****************************************************************************/
plerrcode
gpx_edge_(struct lvd_dev* dev, struct lvd_acard* acard, int channel,
        int edges, int immediate)
{
    struct gpx_info* info=(struct gpx_info*)acard->cardinfo;
    plerrcode pres=plOK;

    if (channel<0) { /* all 64 channels */
        int edgereg, gpx;
        edgereg=info->noedges; /* ROsc and service bits */
        if (edges&1)
            edgereg|=0x007f800;
        if (edges&2)
            edgereg|=0xff00000;
        for (gpx=0; gpx<8; gpx++) {        
            info->edges[gpx]=edgereg;
        }
        if (immediate) {
            DEFINE_LVDTRACE(trace);
            lvd_settrace(dev, &trace, 1);
            if (gpx_reg_w(dev, acard, 8, 0, edgereg)<0)
                pres=plErr_System;
            lvd_settrace(dev, 0, trace);
        }
    } else { /* only 1 channel */
        int edgereg, gpx;
        gpx=channel>>3;
        channel&=7;       /* channel of gpx */
        edgereg=info->edges[gpx];
        /* disable both edges */
        edgereg&=~(0x0100800<<channel);
        /* enable rising edge */
        if (edges&1)
            edgereg|=0x0000800<<channel;
        /* enable falling edge */
        if (edges&2)
            edgereg|=0x0100000<<channel;
        info->edges[gpx]=edgereg;
        if (immediate) {
            DEFINE_LVDTRACE(trace);
            lvd_settrace(dev, &trace, 1);
            if (gpx_reg_w(dev, acard, gpx, 0, edgereg)<0)
                pres=plErr_System;
            lvd_settrace(dev, 0, trace);
        }
    }

    return pres;
}
/*****************************************************************************/
plerrcode
gpx_dac_(struct lvd_dev* dev, struct lvd_acard* acard, int connector, int dac,
        int version)
{
    plerrcode plerr;
    ems_u32 val;
    int i;

    dac&=0xff;

    if (connector<0) {
        for (i=1; i<=8; i++) {
            if ((plerr=gpx_dac_(dev, acard, i, dac, version))!=plOK)
                return plerr;
        }
        return plOK;
    }

    val=(((connector&7)+1)<<8)|dac;
    if (lvd_i_w(dev, acard->addr, gpx.dac_data, val)<0)
        return plErr_System;
    do {
        if (lvd_i_r(dev, acard->addr, gpx.sr, &val)<0)
            return plErr_System;
    } while (val&GPX_SR_DAC_BUSY);

    lvd_i_w(dev, acard->addr, gpx.dac_data, 0);
    do {
        if (lvd_i_r(dev, acard->addr, gpx.sr, &val)<0)
            return plErr_System;
    } while (val&GPX_SR_DAC_BUSY);

    //printf("gpx_dac_[%x]: dac %d %d\n", acard->addr, connector, dac);

    return plOK;
}
/*****************************************************************************/
plerrcode
gpx_mask_(struct lvd_dev *dev, struct lvd_acard *acard, u_int32_t *mask)
{
    struct gpx_info* info=(struct gpx_info*)acard->cardinfo;
    int edgereg, channelbyte, chanmask, gpx;

    /* we have to iterate over the eight GPX gpxs */
    for (gpx=0; gpx<8; gpx++) {
        if (gpx<4)
            channelbyte=(mask[0]>>(8*gpx))&0xff;
        else
            channelbyte=(mask[1]>>(8*(gpx-4)))&0xff;

        chanmask=(channelbyte<<20) | (channelbyte<<11);

        edgereg=info->edges[gpx];
        edgereg&=~chanmask;
        info->edges[gpx]=edgereg;
    }
    return plOK;
}
/*****************************************************************************/
plerrcode
gpx_get_mask_(struct lvd_dev *dev, struct lvd_acard *acard, u_int32_t *mask)
{
    return plErr_NotImpl;
}
/*****************************************************************************/
/*****************************************************************************/
static int
dump_ident(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, gpx.ident, &val)<0) {
        xprintf(xp, "  unable to read ident\n");
        return -1;
    }

    xprintf(xp, "  [00] ident    =0x%04x (type=0x%x version=0x%x)\n",
            val, val&0xff, (val>>8)&0xff);
    return 0;
}
/*****************************************************************************/
static int
dump_serial(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;
    int res;

    dev->silent_errors=1;
    res=lvd_i_r(dev, addr, gpx.serial, &val);
    dev->silent_errors=0;
    if (res<0) {
        xprintf(xp, "  [02] serial   : not set\n");
    } else {
        xprintf(xp, "  [02] serial   =%d\n", val);
    }
    return 0;
}
/*****************************************************************************/
static int
dump_int_err(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, gpx.gpx_int_err, &val)<0) {
        xprintf(xp, "  unable to read gpx_int_err\n");
        return -1;
    }

    xprintf(xp, "  [10] int_err  =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_empty_flags(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, gpx.gpx_empty, &val)<0) {
        xprintf(xp, "  unable to read gpx_empty\n");
        return -1;
    }

    xprintf(xp, "  [12] gpx_empty=0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_lat(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, gpx.trg_lat, &val)<0) {
        xprintf(xp, "  unable to read trg_lat\n");
        return -1;
    }

    xprintf(xp, "  [04] trg_lat  =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_win(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, gpx.trg_win, &val)<0) {
        xprintf(xp, "  unable to read trg_win\n");
        return -1;
    }

    xprintf(xp, "  [06] trg_win  =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_gpx_address(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, gpx.gpx_seladdr, &val)<0) {
        xprintf(xp, "  unable to read gpx_seladdr\n");
        return -1;
    }

    xprintf(xp, "  [24] gpx_addr =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_sr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 sr;

    if (lvd_i_r(dev, addr, gpx.sr, &sr)<0) {
        xprintf(xp, "  unable to read sr\n");
        return -1;
    }

    xprintf(xp, "  [0e] sr       =0x%04x", sr);
    if (sr&GPX_SR_SEQ_ERR)
        xprintf(xp, " SEQ_ERR");
    if (sr&GPX_SR_DAC_BUSY)
        xprintf(xp, " DAC_BUSY");
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
dump_cr(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 cr;

    if (lvd_i_r(dev, addr, gpx.cr, &cr)<0) {
        xprintf(xp, "  unable to read cr\n");
        return -1;
    }

    xprintf(xp, "  [0c] cr       =0x%04x", cr);
    if (cr&GPX_CR_TMATCHING)
        xprintf(xp, " WINDOW");
    if (cr&GPX_CR_RAW)
        xprintf(xp, " RAW");
    if (cr&GPX_CR_LETRA)
        xprintf(xp, " LETRA");
    if (cr&GPX_CR_EXTENDED)
        xprintf(xp, " EXTENDED");
    if (cr&GPX_CR_GPXMODE)
        xprintf(xp, " GPXMODE");
    xprintf(xp, "\n");
    return 0;
}
/*****************************************************************************/
static int
decode_gpx_reg(int gpx, int reg, ems_u32 val, void *xp, int level)
{
    char s[33];
    int i;

    xprintf(xp, "  %2d %07x", reg, val);
    switch (reg) {
    case 0:
        if (val & 1<<0) xprintf(xp, " ROsc");
        if (val & 1<<1) xprintf(xp, " RiseEn0");
        if (val & 1<<2) xprintf(xp, " FallEn0");
        if (val & 1<<3) xprintf(xp, " RiseEn1");
        if (val & 1<<4) xprintf(xp, " FallEn1");
        if (val & 1<<5) xprintf(xp, " RiseEn2");
        if (val & 1<<6) xprintf(xp, " FallEn2");
        if ((val>>7 & 7)!=1)  xprintf(xp, " ServBits=%d", val>>7 & 7);
        sprintf(s, "--________________");
        if (val & 1<<10) s[0]='R';
        if (val & 1<<19) s[1]='F';
        for (i=11; i<=18; i++) {
            if (val & 1<<i) s[2+(i-11)*2]='r';
        }
        for (i=20; i<=27; i++) {
            if (val & 1<<i) s[3+(i-20)*2]='f';
        }
        xprintf(xp, "; edges: %s", s);
        break;
    case 1:
        //xprintf(xp, " (channel adjustment)");
        break;
    case 2:
        if (val & 1<<0) xprintf(xp, " G-Mode");
        if (val & 1<<1) xprintf(xp, " I_Mode");
        if (val & 1<<2) xprintf(xp, " R-Mode");
        sprintf(s, "-________");
        if (val & 1<<3) s[0]='X';
        for (i=4; i<=11; i++) {
            if (val & 1<<i) s[i-3]='x';
        }
        xprintf(xp, "; disabled: %s", s);
        break;
    case 3:
        //xprintf(xp, " (ignored in I-Mode)");
        break;
    case 4:
        xprintf(xp, " StartTimer=%d", val&0xff);
        if (val & 1<<9) xprintf(xp, " M-Mode");
        if (val & 1<<22) xprintf(xp, " MasterReset");
        if (val & 1<<23) xprintf(xp, " PartialReset");
        if (val & 1<<24) xprintf(xp, " AluTrigSoft");
        if (val & 1<<25) xprintf(xp, " EFlagHiZN");
        if (val & 1<<26) xprintf(xp, " MTimerStart");
        if (val & 1<<27) xprintf(xp, " MTimerStop");
        break;
    case 5:
        xprintf(xp, " StartOff1=0x%x", val&0x3ffff);
        if (val & 1<<21) xprintf(xp, " StopDisStart");
        if (val & 1<<22) xprintf(xp, " StartDisStart");
        if (val & 1<<23) xprintf(xp, " MasterAluTrig");
        if (val & 1<<24) xprintf(xp, " PartialAluTrig");
        if (val & 1<<25) xprintf(xp, " MasterOenTrig");
        if (val & 1<<26) xprintf(xp, " PartialOenTrig");
        if (val & 1<<27) xprintf(xp, " StatRetrig");
        break;
    case 6:
        xprintf(xp, " Fill=%d", val&0xff);
        xprintf(xp, " StartOff2=%d", (val>>25)&0x3ffff);
        if (val & 1<<26) xprintf(xp, " InSelECL");
        if (val & 1<<27) xprintf(xp, " PowerOnEcl");
        break;
    case 7:
        xprintf(xp, " HSDiv=0x%x", val&0xff);
        xprintf(xp, " RefClkDiv=%d", (val>>28)&0x7);
        if (val & 1<<11) xprintf(xp, " ResAdj");
        if (val & 1<<12) xprintf(xp, " NegPhase");
        if (val & 1<<13) xprintf(xp, " Track");
        xprintf(xp, " MTimer=%d", (val>>15)&0x1fff);
        break;
    case 8:
    case 9:
    case 10:
        xprintf(xp, " (data)");
        break;
    case 11:
        if (val&0xff) xprintf(xp, " StopCounter0=%d", val&0xff);
        if (val&0xff00) xprintf(xp, " StopCounter1=%d", val>>8 & 0xff);
        if ((val>>16 & 0xff)!=0xff) xprintf(xp, " HFifoErrU=0x%02x", val>>16 & 0xff);
        if ((val>>24 & 0x3)!=0x3) xprintf(xp, " IFifoErrU=0x%x", val>>24 & 0x3);
        if ((val>>26 & 0x1)!=0x1) xprintf(xp, " NotLockErrU=0x%x", val>>26 & 0x1);
        break;
    case 12:
        if (val & 0xff) xprintf(xp, " HFifoFull=0x%02x", val & 0xff);
        if (val>>8 & 0x3) xprintf(xp, " IFifoFull=0x%x", val>>8 & 0x3);
        if (val>>10 & 0x1) xprintf(xp, " NotLockErrU=0x%x", val>>10 & 0x1);
        if (val & 1<<11) xprintf(xp, " HFifoE");
        if (val & 1<<12) xprintf(xp, " TimerFlag");
        if (val>>13 & 0xff) xprintf(xp, " HFifoIntU=0x%02x", val>>13 & 0xff);
        if (val>>21 & 0x3) xprintf(xp, " IFifoIntU=0x%x", val>>21 & 0x3);
        if (val & 1<<23) xprintf(xp, " NotLockIntU");
        if (val & 1<<24) xprintf(xp, " HFifoEU");
        if (val & 1<<25) xprintf(xp, " TimerFlagU");
        if (val & 1<<26) xprintf(xp, " Start#U");
        break;
    }
    xprintf(xp, "\n");

    return 0;
}
/*****************************************************************************/
static int
dump_gpx_regs(struct lvd_dev* dev, int addr, void *xp, int level)
{
    static int regs_to_be_read[]={
        0, 1, 2, 3, 4, 5, 6, 7, 11, 12, -1,
    };
    ems_u32 vals[8][13];
    int gpx, i;

    xprintf(xp, "  gpx registers:\n");
    for (i=0; regs_to_be_read[i]>=0; i++) {
        xprintf(xp, "%7d ", regs_to_be_read[i]);
    }

    xprintf(xp, "\n");
    for (gpx=0; gpx<8; gpx++) {
        for (i=0; regs_to_be_read[i]>=0; i++) {
            ems_u32 val;
            int reg=regs_to_be_read[i];

            if (lvd_i_w(dev, addr, gpx.gpx_seladdr, (gpx<<4)|reg)<0) {
                xprintf(xp, "  unable to write gpx_reg\n");
                return -1;
            }
            if (lvd_i_r(dev, addr, gpx.gpx_data, &val)<0) {
                xprintf(xp, "  unable to read gpx_reg\n");
                return -1;
            }
            vals[gpx][reg]=val;
        }
    }
    for (gpx=0; gpx<8; gpx++) {
        for (i=0; regs_to_be_read[i]>=0; i++) {
            int reg=regs_to_be_read[i];
            ems_u32 val=vals[gpx][reg];

            xprintf(xp, "%s%07x", i?" ":"", val);
        }
        xprintf(xp, "\n");
    }
    if (level>2) {
        for (gpx=0; gpx<8; gpx++) {
            xprintf(xp, "gpx %d\n", gpx);
            for (i=0; regs_to_be_read[i]>=0; i++) {
                int reg=regs_to_be_read[i];
                ems_u32 val=vals[gpx][reg];

                decode_gpx_reg(gpx, reg, val, xp, level);
            }
            xprintf(xp, "\n");
        }
    }

    return 0;
}
/*****************************************************************************/
static int
dump_gpx_range(struct lvd_dev* dev, int addr, void *xp)
{
    ems_u32 val;

    if (lvd_i_r(dev, addr, gpx.gpx_range, &val)<0) {
        xprintf(xp, "  unable to read gpx_range\n");
        return -1;
    }

    xprintf(xp, "  [4c] gpx_range =0x%04x\n", val);
    return 0;
}
/*****************************************************************************/
static int
dump_dcm_shift(struct lvd_dev* dev, struct lvd_acard* acard, void *xp)
{
    struct gpx_info* info=(struct gpx_info*)acard->cardinfo;

    xprintf(xp, "  dcm_shift0=%d\n", info->dcm_shift[0]);
    xprintf(xp, "  dcm_shift1=%d\n", info->dcm_shift[1]);
    return 0;
}
/*****************************************************************************/
static int
dump_softinfo(struct lvd_dev* dev, struct lvd_acard* acard, void *xp)
{
    xprintf(xp, "  daq_mode  =0x%04x\n", acard->daqmode);
    return 0;
}
/*****************************************************************************/
static int
lvd_cardstat_gpx(struct lvd_dev* dev, struct lvd_acard* acard, void *xp,
    int level)
{
    int addr=acard->addr;

    xprintf(xp, "ACQcard 0x%03x GPX:\n", addr);

    switch (level) {
    case 0:
        dump_cr(dev, addr, xp);
        dump_sr(dev, addr, xp);
        dump_int_err(dev, addr, xp);
        dump_empty_flags(dev, addr, xp);
        break;
    case 1:
    case 2:
    default:
        dump_ident(dev, addr, xp);
        dump_serial(dev, addr, xp);
        dump_lat(dev, addr, xp);
        dump_win(dev, addr, xp);
        dump_cr(dev, addr, xp);
        dump_sr(dev, addr, xp);
        dump_int_err(dev, addr, xp);
        dump_empty_flags(dev, addr, xp);
        dump_gpx_address(dev, addr, xp);
        dump_gpx_regs(dev, addr, xp, level);
        dump_gpx_range(dev, addr, xp);
        dump_dcm_shift(dev, acard, xp);
        dump_softinfo(dev, acard, xp);
        break;
    }

    return 0;
}
/*****************************************************************************/
static void
lvd_gpx_acard_free(struct lvd_dev* dev, struct lvd_acard* acard)
{
    free(acard->cardinfo);
    acard->cardinfo=0;
}
/*****************************************************************************/
int
lvd_gpx_acard_init(struct lvd_dev* dev, struct lvd_acard* acard)
{
    struct gpx_info *info;

    acard->freeinfo=lvd_gpx_acard_free;
    acard->clear_card=lvd_clear_gpx;
    acard->start_card=lvd_start_gpx;
    acard->stop_card=lvd_stop_gpx;
    acard->cardstat=lvd_cardstat_gpx;
    acard->cardinfo=malloc(sizeof(struct gpx_info));
    if (!acard->cardinfo) {
        printf("malloc acard->cardinfo(gpx): %s\n", strerror(errno));
        return -1;
    }
    info=acard->cardinfo;
    info->dcm_shift[0]=0;
    info->dcm_shift[1]=0;
    
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
