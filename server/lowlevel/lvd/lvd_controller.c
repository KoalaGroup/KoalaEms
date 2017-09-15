/*
 * lowlevel/lvd/lvd_controller.c
 * created 2005-Feb-23 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_controller.c,v 1.29 2012/09/11 23:38:24 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "lvdbus.h"
#include "lvd_access.h"
#include "lvd_map.h"
#include "../oscompat/oscompat.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd")

/*****************************************************************************/
#ifndef LVD_TRACE
static int
f1_reg_wait(struct lvd_dev* dev)
{
    ems_u32 val;
    int weiter, res=0;

    do {
        res|=lvd_c1_r(dev, sr, &val);
        weiter=val&F1_WRBUSY;
    } while (!res && weiter);
    return res;
}
#else
static int
f1_reg_wait(struct lvd_dev* dev)
{
    DEFINE_LVDTRACE(trace);
    ems_u32 val;
    int weiter, res=0;
    int n=0;

    lvd_settrace(dev, &trace, -1);
    do {
        if (n)
            lvd_settrace(dev, 0, 0);
        res|=lvd_c1_r(dev, sr, &val);
        weiter=val&F1_WRBUSY;
        n++;
    } while (!res && weiter);
    if (LVDTRACE_ACTIVE(trace))
        printf("lvd_controller.c: f1_reg_wait: %d\n", n);
    lvd_settrace(dev, 0, trace);
    return res;
}
#endif
/*****************************************************************************/
static int
f1_reg_w(struct lvd_dev* dev, int unsigned reg, ems_u32 val)
{
    if (dev->ccard.contr_type==contr_f1) {
        if (lvd_c1_w(dev, f1_reg[reg], val)<0)
            return -1;
        if (f1_reg_wait(dev))
            return -1;
    } else {
        if (lvd_c3_w(dev, f1_reg[reg], val)<0)
            return -1;
    }
    dev->ccard.f1_shadow[reg]=val;
    return 0;
}
/*****************************************************************************/
static int __attribute__((unused))
f1_reg_r(struct lvd_dev* dev, int unsigned reg, ems_u32* val)
{
    *val=dev->ccard.f1_shadow[reg];
    return 0;
}
/*****************************************************************************/
static int
gpx_reg_w(struct lvd_dev* dev, int unsigned reg, ems_u32 val)
{
    printf("gpx write: reg %2d val %07x\n", reg, val);
    if (dev->ccard.contr_type==contr_gpx) {
        if (lvd_c2_w(dev, gpx_reg, reg)<0)
            return -1;
        if (lvd_c2_w(dev, gpx_data, val)<0)
            return -1;
    } else {
        if (lvd_c3_w(dev, gpx_reg, reg)<0)
            return -1;
        if (lvd_c3_w(dev, gpx_data, val)<0)
            return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
gpx_reg_r(struct lvd_dev* dev, int unsigned reg, ems_u32* val)
{
    if (dev->ccard.contr_type==contr_gpx) {
        if (lvd_c2_w(dev, gpx_reg, reg)<0)
            return -1;
        if (lvd_c2_r(dev, gpx_data, val)<0)
            return -1;
    } else {
        if (lvd_c3_w(dev, gpx_reg, reg)<0)
            return -1;
        if (lvd_c3_r(dev, gpx_data, val)<0)
            return -1;
    }
    return 0;
}
/*****************************************************************************/
static plerrcode
lvd_init_controller_f1gpx_f1(struct lvd_dev* dev, int input, int hsdiv,
    int edge)
{
    ems_u32 val, oval, reg_2, reg_3;
    int i, f1_range;
    float resolution;
    DEFINE_LVDTRACE(trace);

    printf("lvd_init_controller for F1, input=0x%x\n", input);

    if (hsdiv<0) {
        /* if we mix F1 and GPX modules hsdiv has to be set to 0xa2 */
        hsdiv=0xa2; /* 129.9545 ps */
    } else if (hsdiv==0) {
        hsdiv=0xaf; /* 120.3 ps */
    } else {
        /* hsdiv is set to the requested value
           but then we cannot use old GPX modules */
    }
    resolution=F1_HSDIV/hsdiv;
    f1_range=304*hsdiv; /* == 6.4us/resolution */
    printf("hsdiv     =%d (0x%x)\n", hsdiv, hsdiv);
    printf("resolution=%.2f ps\n", resolution);
    printf("f1_range  =%d (0x%x)\n", f1_range, f1_range);
    dev->hsdiv=hsdiv;
    dev->tdc_range=f1_range;
    dev->tdc_resolution=resolution;

    /* initialize system controller */
    /* DAQ mode "idle" */
    if (lvd_c3_w(dev, cr, 0)<0) return plErr_HW;

    /*if (lvd_c3_w(dev, ctrl, PURES)<0) return plErr_HW;*/
    if (f1_reg_w(dev, 7, 0)<0) return plErr_HW;
    if (lvd_c3_r(dev, sr, &val)<0) return plErr_HW;
    if (!(val&F1_NOINIT)) {
        printf("lvd_init_controller: F1 still initialized.\n");
        return plErr_HW;
    }

    if (f1_reg_w(dev, 15, F1_R15_ROSTA|F1_R15_COM|F1_R15_8BIT)<0)
        return plErr_HW;
    if (f1_reg_w(dev, 10, 0x1800|(F1_REFCLKDIV<<8)|hsdiv)<0) return plErr_HW;

    if (f1_reg_w(dev, 0, 0)<0) return plErr_HW;
    if (f1_reg_w(dev, 1, 0)<0) return plErr_HW;

    dev->ccard.trigger_edge=edge;
    dev->ccard.trigger_mask=input;
    reg_2=0;
    reg_3=0;
    if (input&1) /* NIM */
        reg_2|=0x0080;
    if (input&2) /* RJ45 */
        reg_2|=0x8000;
    if (input&4) /* COAX */
        reg_3|=0x0080;
    if (input&8) /* OPTICS */
        reg_3|=0x8000;

/*  This is not forseen in controler FPGA 10.05.2012 (PK)
    if (edge) {
        reg_2>>=1;
        reg_3>>=1;
    }
*/

    if (f1_reg_w(dev, 2, reg_2)<0) return plErr_HW;
    if (f1_reg_w(dev, 3, reg_3)<0) return plErr_HW;
    /* all other channels disabled */
    if (f1_reg_w(dev, 4, 0x0000)<0) return plErr_HW;
    if (f1_reg_w(dev, 5, 0x0000)<0) return plErr_HW;

    if (f1_reg_w(dev, 7, F1_R7_BEINIT)<0) return plErr_HW;
/* wait for PLL lock */
    if (lvd_c3_r(dev, sr, &oval)<0) return plErr_HW;
    lvd_settrace(dev, &trace, 0);
    for (i=0; i<50; i++) {
        tsleep(1); /* sleep 10 ms */
        if (lvd_c3_r(dev, sr, &val)<0) {
            lvd_settrace(dev, 0, trace);
            return plErr_HW;
        }
        if (val!=oval) {
            printf("f1_init scm: sr changed from 0x%04x to 0x%04x at loop %d\n",
                oval, val, i);
            oval=val;
        }
    }
    lvd_settrace(dev, 0, trace);
/* XXX reinit??? */
    if (val&(F1_NOLOCK|F1_NOINIT)) {
        printf("f1_init scm: sr=0x%04x:\n", val);
        if (val&F1_NOLOCK)
            printf(" NOLOCK");
        if (val&F1_NOINIT)
            printf(" NOINIT");
        printf("\n");
        return plErr_HW;
    } else {
        printf("f1_init scm: sr=0x%04x (OK)\n", val);
    }
    printf("lvd_init_controller: F1GPX system controller initialized with F1.\n");
    return plOK;
}
/*****************************************************************************/
static plerrcode
lvd_init_controller_f1gpx_gpx(struct lvd_dev* dev, int input, int hsdiv,
    int edge)
{
    ems_u32 val, redges, fedges, reg0, reg4, reg5, reg7, i;
    int gpx_range;
    float resolution;

    printf("lvd_init_controller for GPX, input=0x%x\n", input);

    if (hsdiv<0) {
        printf("lvd_init_controller: F1 mode not supported\n");
        return plErr_ArgRange;
    } else if (hsdiv==0) {
        hsdiv=0xa0; /* 92.59 ps */
    } else {
        /* hsdiv is set to the requested value */
    }
    resolution=GPX_HSDIV/hsdiv;
    gpx_range=432*hsdiv; /* == 6.4us/resolution */
    printf("hsdiv     =%d (0x%x)\n", hsdiv, hsdiv);
    printf("resolution=%.2f ps\n", resolution);
    printf("gpx_range =%d (0x%x)\n", gpx_range, gpx_range);
    dev->hsdiv=hsdiv;
    dev->tdc_range=gpx_range;
    dev->tdc_resolution=resolution;

    /* initialize system controller */
    /* DAQ mode "idle" */
    if (lvd_c3_w(dev, cr, 0)<0) return plErr_HW;

    /* reset */
    if (lvd_c3_w(dev, ctrl, LVD_PURES)<0) return plErr_HW;
    if (gpx_reg_r(dev, 0, &val)<0) return plErr_HW;
    if (val!=0x80) {
        printf("lvd_init_controller_3: invalid GPX status after reset\n");
        return plErr_HW;
    }

    lvd_c3_w(dev, gpx_range, dev->tdc_range);

    /* set I-Mode */
    if (gpx_reg_w(dev, 2, 0x2)<0) return plErr_HW;

/*
 *     trigger inputs:
 *     0x1: LEMO, 0x2: RJ45, 0x4: 6pack, 0x8: optic
 */
    dev->ccard.trigger_edge=edge;
    dev->ccard.trigger_mask=input;
    if (edge) { /* leading edge */
        redges=input&0xf;
        fedges=input&0x0;
    } else {
        fedges=input&0xf;
        redges=input&0x0;
    }
    reg0=0x0000481|redges<<11|fedges<<20;
    if (gpx_reg_w(dev, 0, reg0)<0) return plErr_HW;

    reg7=0x0001800L|(GPX_REFCLKDIV<<8)|hsdiv;
    if (gpx_reg_w(dev, 7, reg7)<0) return plErr_HW;

    reg4=0x2000001;
    if (gpx_reg_w(dev, 4, reg4)<0) return plErr_HW;


    /*reg5=config.g_startoff1|(f1_sync<2?0x2000000:0xA000000);*/
    /*config.g_startoff1=0x20000*/
    reg5=0xA020000;
    if (gpx_reg_w(dev, 5, reg5)<0) return plErr_HW;


    /* enable err flag "PLL not locked" */
    if (gpx_reg_w(dev, 11, 0x4000000)<0) return plErr_HW;

    /* enable start high to interrupt */
    if (gpx_reg_w(dev, 12, 0x4000000)<0) return plErr_HW;

    reg4|=0x2400000; /* Master Reset */
    if (gpx_reg_w(dev, 4, reg4)<0) return plErr_HW;

    i=0;
    do {
        if (gpx_reg_r(dev, 12, &val)<0) return plErr_HW;
    } while ((val&0x0400) && (++i<10000));
    if (val&0x0400) {
        printf("lvd_init_controller_2: no PLL lock\n");
        return plErr_HW;
    }

    printf("lvd_init_controller: F1GPX system controller initialized with GPX.\n");
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_controller_f1gpx_set_rate_trigger_count count value for rate trigger
 */
plerrcode
lvd_controller_f1gpx_set_rate_trigger_count(struct lvd_dev* dev, int count)
{
    ems_u32 val;

    dev->ccard.rate_trigger_count=count;

    if (lvd_c3_r(dev, rate_trigger, &val)<0) {
        printf("== lowlevel/lvd/lvd_controller.c:");
        printf(" lvd_controller_f1gpx_set_rate_trigger_count F1GPX ==\n");
        printf(" unable to read rate_triger value\n");
        return  plErr_HW;;
    }

    count= (count & 0xff) << 8 ;
    val=(val & 0xff) | count;

    if (lvd_c3_w(dev, rate_trigger, val )<0) {
        printf("== lowlevel/lvd/lvd_controller.c:");
        printf(" lvd_controller_f1gpx_set_rate_trigger_count F1GPX ==\n");
        printf("  count=0x%x\n", count);
        printf(" unable to set rate_triger value\n");
        return plErr_HW;
    }
    printf("lvd_controller_f1gpx_set_rate_trigger_count count set to 0x%x\n",
            count);
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_controller_f1gpx_get_rate_trigger_count count value for rate trigger
 */
plerrcode
lvd_controller_f1gpx_get_rate_trigger_count(struct lvd_dev* dev, ems_u32* count)
{
    if (lvd_c3_r(dev, rate_trigger, count )<0) {
        printf("== lowlevel/lvd/lvd_controller.c:");
        printf(" lvd_controller_f1gpx_get_rate_trigger_count F1GPX ==\n");
        printf(" unable to read  rate_triger value\n");
        return plErr_HW;
    }
    *count=(*count & 0xff00)>>8;
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_controller_f1gpx_set_rate_trigger_period period value for rate trigger
 */
plerrcode
lvd_controller_f1gpx_set_rate_trigger_period(struct lvd_dev* dev, int period)
{
    ems_u32 val;

    dev->ccard.rate_trigger_period=period;

    if (lvd_c3_r(dev, rate_trigger, &val)<0) {
        printf("== lowlevel/lvd/lvd_controller.c:");
        printf(" lvd_controller_f1gpx_set_rate_trigger_period F1GPX ==\n");
        printf("  unable to read  rate_triger value\n");
        return  plErr_HW;;
    }

    period= (period & 0xff)  ;
    val=(val & 0xff00) | period;

    if (lvd_c3_w(dev, rate_trigger, val ) <0) {
       printf("== lowlevel/lvd/lvd_controller.c:");
       printf(" lvd_controller_f1gpx_set_rate_trigger_period F1GPX ==\n");
       printf("  period=0x%x\n", period);
       printf("  unable to set rate_triger value\n");
       return plErr_HW;
    }
    printf("lvd_controller_f1gpx_set_rate_trigger_period period set to 0x%x\n",
            period);
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_controller_f1gpx_get_rate_trigger_period period  value for rate trigger
 */
plerrcode
lvd_controller_f1gpx_get_rate_trigger_period(struct lvd_dev* dev,
        ems_u32* period)
{
    printf("== lowlevel/lvd/lvd_controller.c:");
    printf(" lvd_controller_f1gpx_get_rate_trigger_count F1GPX ==\n");

    if (lvd_c3_r(dev, rate_trigger, period)<0) {
        printf("== lowlevel/lvd/lvd_controller.c:");
        printf(" lvd_controller_f1gpx_get_rate_trigger_count F1GPX ==\n");
        printf(" unable to read  rate_triger value\n");
        return plErr_HW;
    }
    *period=(*period & 0xff00) >> 8;
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_controller_f1gpx_extra_trig Append trigger data to readout data
 */
plerrcode
lvd_controller_f1gpx_extra_trig(struct lvd_dev* dev, int extra_trig)
{
    dev->ccard.extra_trig=extra_trig;
    ems_u32 val;

    if (lvd_cc_r(dev, cr, &val)<0) {
        printf("== lowlevel/lvd/lvd_controller.c:");
        printf(" lvd_controller_f1gpx_extra_trig F1GPX ==\n");
        printf("  unable to read cr value\n");
        return  plErr_HW;;
    }
    if (extra_trig) {
        if (lvd_c3_w(dev, cr, val|LVD_MCR3_EXTRA_TRIG)) {
            printf("== lowlevel/lvd/lvd_controller.c:");
            printf(" lvd_controller_f1gpx_extra_trig F1GPX ==\n");
            printf("  extra_trig=0x%x\n", extra_trig);
            printf("  unable to set EXTRA_TRIG ON write failed\n");
            return plErr_HW;
        }
        printf("lvd_controller_f1gpx_extra_trig EXTRA_TRIG  ON \n");
    } else {
        if (lvd_c3_w(dev, cr, val|!LVD_MCR3_EXTRA_TRIG)) {
           printf("== lowlevel/lvd/lvd_controller.c:");
           printf(" lvd_controller_f1gpx_extra_trig F1GPX ==\n");
           printf("  extra_trig=0x%x\n", extra_trig);
           printf("  unable to set EXTRA_TRIG OFF write failed\n");
           return plErr_HW;
        }
        printf("lvd_controller_f1gpx_extra_trig EXTRA_TRIG OFF \n");
    }
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_controller_f1gpx_rate_trig Switch rate trigger
 */
plerrcode
lvd_controller_f1gpx_rate_trig(struct lvd_dev* dev, int rate_trig)
{
    dev->ccard.rate_trig=rate_trig;

    ems_u32 val;
    if (lvd_cc_r(dev, cr, &val)<0) {
        printf("== lowlevel/lvd/lvd_controller.c:");
        printf(" lvd_controller_f1gpx_rate_trig F1GPX ==\n");
        printf("  unable to read cr value\n");
        return  plErr_HW;;
    } 
    if (rate_trig) {
        if (lvd_c3_w(dev, cr, val|LVD_MCR3_RATE_TRIG)) {
            printf("== lowlevel/lvd/lvd_controller.c:");
            printf(" lvd_controller_f1gpx_rate_trig F1GPX ==\n");
            printf("  rate_trig=0x%x\n", rate_trig);
            printf("  unable to set RATE_TRIG ON write failed\n");
            return plErr_HW;
        }
        printf("lvd_controller_f1gpx_rate_trig: RATE_TRIG ON \n");
    } else {
        if (lvd_c3_w(dev, cr, val|!LVD_MCR3_RATE_TRIG)) {
            printf("== lowlevel/lvd/lvd_controller.c:");
            printf(" lvd_controller_f1gpx_rate_trig F1GPX ==\n");
            printf("  rate_trig=0x%x\n", rate_trig);
            printf("  unable to set RATE_TRIG OFF write failed\n");
            return plErr_HW;
        }
        printf("lvd_controller_f1gpx_rate_trig :OFF \n");
    }
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_init_controller_3 initializes a LVD crate controller with F1 and GPX chip
 */
static plerrcode
lvd_init_controller_f1gpx(struct lvd_dev* dev, int input, int hsdiv, int edge,
    int mode)
{
    plerrcode pres;
    int i;

    printf("== lowlevel/lvd/lvd_controller.c: lvd_init_controller F1GPX ==\n");
    printf("  input=0x%x\n", input);

    dev->ccard.contr_type=contr_f1gpx;
    switch (mode) {
    case 0:
        dev->contr_mode=contr_f1;
        break;
    case 1:
        dev->contr_mode=contr_gpx;
        break;
    default:
        dev->contr_mode=contr_gpx;
        for (i=0; i<dev->num_acards; i++) {
            if (dev->acards[i].mtype==ZEL_LVD_TDC_F1) {
                dev->contr_mode=contr_f1;
                break;
            }
        }
    }

    if (dev->contr_mode==contr_gpx) {
        pres=lvd_init_controller_f1gpx_gpx(dev, input, hsdiv, edge);
    } else {
        pres=lvd_init_controller_f1gpx_f1(dev, input, hsdiv, edge);
    }
    return pres;
}
/*****************************************************************************/
/*
 * lvd_init_controller_2 initializes a LVD crate controller with GPX chip
 * current version is 0x0101
 */

#if (GPX_T_REF!=25) || (GPX_REFCLKDIV!=7)
#error illegal setup of GPX__T_REF or GPX_REFCLKDIV
#endif

static plerrcode
lvd_init_controller_gpx(struct lvd_dev* dev, int input, int hsdiv, int edge)
{
    ems_u32 val, redges, fedges, reg0, reg4, reg5, reg7, i;
    int f1_range, gpx_range;
    float resolution;

    printf("== lowlevel/lvd/lvd_controller.c: lvd_init_controller GPX ==\n");
    printf("  input=0x%x\n", input);

    dev->ccard.contr_type=contr_gpx;
    dev->contr_mode=contr_gpx;
    if (hsdiv<0) {
        /* if we mix F1 and GPX modules hsdiv has to be set to 0x98 */
        dev->contr_mode=contr_f1;
        hsdiv=0x98; /* 97.47 ps */
    } else if (hsdiv==0) {
        hsdiv=0xa0; /* 92.59 ps */
    } else {
        /* hsdiv is set to the requested value
           but then we cannot use F1 modules */
    }
    resolution=GPX_HSDIV/hsdiv;
    gpx_range=432*hsdiv; /* == 6.4us/resolution */
    f1_range=(gpx_range*3)/4; /* == 6.4us/(resolution*4/3) */
    printf("hsdiv     =%d (0x%x)\n", hsdiv, hsdiv);
    printf("resolution=%.2f ps\n", resolution);
    if (dev->contr_mode==contr_f1)
        printf("tweaked to=%.2f ps\n", resolution*4./3.);
    printf("f1_range  =%d (0x%x)\n", f1_range, f1_range);
    printf("gpx_range =%d (0x%x)\n", gpx_range, gpx_range);
    dev->hsdiv=hsdiv;
    if (dev->contr_mode==contr_f1) {
        dev->tdc_range=f1_range;
        dev->tdc_resolution=resolution*4./3;
    } else {
        dev->tdc_range=gpx_range;
        dev->tdc_resolution=resolution;
    }

    /* initialize system controller */
    /* DAQ mode "idle" */
    if (lvd_c2_w(dev, cr, 0)<0) return plErr_HW;

    /* reset */
    if (lvd_c2_w(dev, ctrl, LVD_PURES)<0) return plErr_HW;
    if (gpx_reg_r(dev, 0, &val)<0) return plErr_HW;
    if (val!=0x80) {
        printf("lvd_init_controller_2: invalid GPX status after reset\n");
        return plErr_HW;
    }

    lvd_c2_w(dev, gpx_range, dev->tdc_range);

    /* set I-Mode */
    if (gpx_reg_w(dev, 2, 0x2)<0) return plErr_HW;

/*
 *     trigger inputs:
 *     0x1: LEMO, 0x2: RJ45, 0x4: 6pack, 0x8: optic
 */
    dev->ccard.trigger_edge=edge;
    dev->ccard.trigger_mask=input;
    if (edge) { /* leading edge */
        redges=input&0xe; /* ECL coax and RJ45 and fiber optics */
        fedges=input&0x1; /* LEMO input is inverted */
    } else {
        fedges=input&0xe;
        redges=input&0x1;
    }
    reg0=0x0000481|redges<<11|fedges<<20;
    if (gpx_reg_w(dev, 0, reg0)<0) return plErr_HW;

    reg7=0x0001800L|(GPX_REFCLKDIV<<8)|hsdiv;
    if (gpx_reg_w(dev, 7, reg7)<0) return plErr_HW;

    reg4=0x2000001;
    if (gpx_reg_w(dev, 4, reg4)<0) return plErr_HW;


    /*reg5=config.g_startoff1|(f1_sync<2?0x2000000:0xA000000);*/
    /*config.g_startoff1=0x20000*/
    reg5=0xA020000;
    if (gpx_reg_w(dev, 5, reg5)<0) return plErr_HW;


    /* enable err flag "PLL not locked" */
    if (gpx_reg_w(dev, 11, 0x4000000)<0) return plErr_HW;

    /* enable start high to interrupt */
    if (gpx_reg_w(dev, 12, 0x4000000)<0) return plErr_HW;

    reg4|=0x2400000; /* Master Reset */
    if (gpx_reg_w(dev, 4, reg4)<0) return plErr_HW;

    i=0;
    do {
        if (gpx_reg_r(dev, 12, &val)<0) return plErr_HW;
    } while ((val&0x0400) && (++i<10000));
    if (val&0x0400) {
        printf("lvd_init_controller_2: no PLL lock\n");
        return plErr_HW;
    }

    printf("lvd_init_controller: GPX system controller initialized.\n");
    return plOK;
}
/*****************************************************************************/
/*
 * lvd_init_controller_1 initializes a LVD crate controller with F1 chip
 * current version is 0x3012
 */

#if (F1_T_REF!=25) || (F1_REFCLKDIV!=7)
#error illegal setup of F1_T_REF or F1_REFCLKDIV
#endif

static plerrcode
lvd_init_controller_f1(struct lvd_dev* dev, int hsdiv, int edge)
{
    ems_u32 val, oval;
    int i, f1_range;
    float resolution;
    DEFINE_LVDTRACE(trace);

    printf("== lowlevel/lvd/lvd_controller.c: lvd_init_controller F1 ==\n");

    dev->ccard.contr_type=contr_f1;
    dev->contr_mode=contr_f1;
    if (hsdiv<0) {
        /* if we mix F1 and GPX modules hsdiv has to be set to 0xa2 */
        hsdiv=0xa2; /* 129.9545 ps */
    } else if (hsdiv==0) {
        hsdiv=0xaf; /* 120.3 ps */
    } else {
        /* hsdiv is set to the requested value
           but then we cannot use old GPX modules */
    }
    resolution=F1_HSDIV/hsdiv;
    f1_range=304*hsdiv; /* == 6.4us/resolution */
    printf("hsdiv     =%d (0x%x)\n", hsdiv, hsdiv);
    printf("resolution=%.2f ps\n", resolution);
    printf("f1_range  =%d (0x%x)\n", f1_range, f1_range);
    dev->hsdiv=hsdiv;
    dev->tdc_range=f1_range;
    dev->tdc_resolution=resolution;

    /* initialize system controller */
    /* DAQ mode "idle" */
    if (lvd_c1_w(dev, cr, 0)<0) return plErr_HW;

    /*if (lvd_c1_w(dev, ctrl, PURES)<0) return plErr_HW;*/
    if (f1_reg_w(dev, 7, 0)<0) return plErr_HW;
    if (lvd_c1_r(dev, sr, &val)<0) return plErr_HW;
    if (!(val&F1_NOINIT)) {
        printf("lvd_init_controller: F1 still initialized.\n");
        return plErr_HW;
    }

    if (f1_reg_w(dev, 15, F1_R15_ROSTA|F1_R15_COM|F1_R15_8BIT)<0)
        return plErr_HW;
    if (f1_reg_w(dev, 10, 0x1800|(F1_REFCLKDIV<<8)|hsdiv)<0) return plErr_HW;

    if (f1_reg_w(dev, 0, 0)<0) return plErr_HW;
    if (f1_reg_w(dev, 1, 0)<0) return plErr_HW;

    dev->ccard.trigger_edge=edge;
    if (edge) {
        /* rising edge */
        if (f1_reg_w(dev, 2, 0x0040)<0) return plErr_HW;
    } else {
        /* falling edge */
        if (f1_reg_w(dev, 2, 0x0080)<0) return plErr_HW;
    }
    /* all other channels disabled */
    if (f1_reg_w(dev, 3, 0x0000)<0) return plErr_HW;
    if (f1_reg_w(dev, 4, 0x0000)<0) return plErr_HW;
    if (f1_reg_w(dev, 5, 0x0000)<0) return plErr_HW;

    if (f1_reg_w(dev, 7, F1_R7_BEINIT)<0) return plErr_HW;
/* wait for PLL lock */
    if (lvd_c1_r(dev, sr, &oval)<0) return plErr_HW;
    lvd_settrace(dev, &trace, 0);
    for (i=0; i<50; i++) {
        tsleep(1); /* sleep 10 ms */
        if (lvd_c1_r(dev, sr, &val)<0) {
            lvd_settrace(dev, 0, trace);
            return plErr_HW;
        }
        if (val!=oval) {
            printf("f1_init scm: sr changed from 0x%04x to 0x%04x at loop %d\n",
                oval, val, i);
            oval=val;
        }
    }
    lvd_settrace(dev, 0, trace);
/* XXX reinit??? */
    if (val&(F1_NOLOCK|F1_NOINIT)) {
        printf("f1_init scm: sr=0x%04x:\n", val);
        if (val&F1_NOLOCK)
            printf(" NOLOCK");
        if (val&F1_NOINIT)
            printf(" NOINIT");
        printf("\n");
        return plErr_HW;
    } else {
        printf("f1_init scm: sr=0x%04x (OK)\n", val);
    }
    printf("lvd_init_controller: F1 system controller initialized.\n");
    return plOK;
}
/*****************************************************************************/
plerrcode
lvd_init_controllers(struct lvd_dev* dev, int input, int hsdiv, int edge,
    int mode)
{
    plerrcode pres;

    switch (LVD_HWtyp(dev->ccard.id)) {
    case LVD_CARDID_CONTROLLER_GPX:
        pres=lvd_init_controller_gpx(dev, input, hsdiv, edge);
        break;
    case LVD_CARDID_CONTROLLER_F1:
        pres=lvd_init_controller_f1(dev, hsdiv, edge);
        break;
    case LVD_CARDID_CONTROLLER_F1GPX:
        pres=lvd_init_controller_f1gpx(dev, input, hsdiv, edge, mode);
        break;
    default:
        printf("lvd_init_controllers: controller is of unknown type %d\n",
                LVD_HWtyp(dev->ccard.id));
        pres=plErr_BadModTyp;
    }

    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_start_controller_1(struct lvd_dev* dev) /* F1 */
{
    int res=0;
    printf("write daqmode 0x%x to F1 controller in LVD crate %d\n",
            dev->ccard.daqmode, dev->generic.dev_idx);
    /* synchronize F1 and 6.4us counter of all modules */
    dev->ccard.last_ev_time[0]=0;
    dev->ccard.last_ev_time[1]=0;
    res|=lvd_cc_w(dev, ctrl, LVD_SYNC_RES);
    /* enable readout */
    res|=lvd_cc_w(dev, cr, dev->ccard.daqmode);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
static plerrcode
lvd_start_controller_2(struct lvd_dev* dev) /* GPX */
{
    int res=0;
    if (dev->contr_mode==contr_f1)
        dev->ccard.daqmode|=0x20;

    printf("write daqmode 0x%x to GPX controller in LVD crate %d\n",
            dev->ccard.daqmode, dev->generic.dev_idx);
    /* synchronize GPX and 6.4us counter of all modules */
    dev->ccard.last_ev_time[0]=0;
    dev->ccard.last_ev_time[1]=0;
    res|=lvd_cc_w(dev, ctrl, LVD_SYNC_RES);
    /* enable readout */
    res|=lvd_cc_w(dev, cr, dev->ccard.daqmode);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
static plerrcode
lvd_start_controller_3(struct lvd_dev* dev) /* F1GPX */
{
    int res=0;
    ems_u32 mode;
    /* synchronize TDC chips and 6.4us counter of all modules */
    dev->ccard.last_ev_time[0]=0;
    dev->ccard.last_ev_time[1]=0;
    res|=lvd_cc_w(dev, ctrl, LVD_SYNC_RES);
    /* enable readout */
    mode=dev->ccard.daqmode&~0x60;
    if (dev->contr_mode==contr_f1)
        mode|=LVD_MCR_F1_MODE;
    printf("write daqmode 0x%x to F1GPX controller in LVD crate %d\n",
            mode, dev->generic.dev_idx);
    res|=lvd_cc_w(dev, cr, mode);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode
lvd_start_controller(struct lvd_dev* dev)
{
    plerrcode pres;

    switch (LVD_HWtyp(dev->ccard.id)) {
    case LVD_CARDID_CONTROLLER_GPX:
        pres=lvd_start_controller_2(dev);
        break;
    case LVD_CARDID_CONTROLLER_F1:
        pres=lvd_start_controller_1(dev);
        break;
    case LVD_CARDID_CONTROLLER_F1GPX:
        pres=lvd_start_controller_3(dev);
        break;
    default:
        printf("lvd_start_controller: controller is of unknown type %d\n",
                LVD_HWtyp(dev->ccard.id));
        pres=plErr_BadModTyp;
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_stop_controller_1(struct lvd_dev* dev) /* F1 */
{
    int res=0;
    printf("write daqmode 0 to F1 controller in LVD crate %d\n",
            dev->generic.dev_idx);
    res|=lvd_cc_w(dev, cr, 0);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
static plerrcode
lvd_stop_controller_2(struct lvd_dev* dev) /* GPX */
{
    int res=0;
    printf("write daqmode 0 to GPX controller in LVD crate %d\n",
            dev->generic.dev_idx);
    res|=lvd_cc_w(dev, cr, 0);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
static plerrcode
lvd_stop_controller_3(struct lvd_dev* dev) /* F1GPX */
{
    int res=0;
    printf("write daqmode 0 to F1GPX controller in LVD crate %d\n",
            dev->generic.dev_idx);
    res|=lvd_cc_w(dev, cr, 0);
    return res?plErr_System:plOK;
}
/*****************************************************************************/
plerrcode
lvd_stop_controller(struct lvd_dev* dev)
{
    plerrcode pres;

    switch (LVD_HWtyp(dev->ccard.id)) {
    case LVD_CARDID_CONTROLLER_GPX:
        pres=lvd_stop_controller_2(dev);
        break;
    case LVD_CARDID_CONTROLLER_F1:
        pres=lvd_stop_controller_1(dev);
        break;
    case LVD_CARDID_CONTROLLER_F1GPX:
        pres=lvd_stop_controller_3(dev);
        break;
    default:
        printf("lvd_stop_controller: controller is of unknown type %d\n",
                LVD_HWtyp(dev->ccard.id));
        pres=plErr_BadModTyp;
    }
    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
