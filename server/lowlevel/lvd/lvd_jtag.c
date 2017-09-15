/*
 * lowlevel/lvd/lvd_jtag.c
 * created 2005-Aug-03 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_jtag.c,v 1.19 2011/04/06 20:30:25 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "lvdbus.h"
#include "lvd_access.h"
#include "lvd_map.h"
#include "../jtag/jtag_lowlevel.h"
#include "dev/pci/sis1100_map.h"

#ifdef LVD_SIS1100
#   include "sis1100/sis1100_lvd.h"
#endif
#ifdef DMALLOC
#   include <dmalloc.h>
#endif

struct opaque_data {
    ems_u32 ident;
    ems_u32 mtype;
    int qdc3;
    int idx;
};

struct opaque_data_sis1100 {
    int p;
};

RCS_REGISTER(cvsid, "lowlevel/lvd")

/****************************************************************************/
static int
jt_action(struct jtag_dev* jdev, int tms, int tdi, int* tdo)
{
    struct opaque_data* opaque=(struct opaque_data*)jdev->opaque;
    struct lvd_dev* dev=(struct lvd_dev*)jdev->dev;
    ems_u32 csr, tdo_=0;
    int res=0;

    if (jdev->ci) {
        csr=LVDMC_JT_C|(tms?LVDMC_JT_TMS:0)|(tdi?LVDMC_JT_TDI:0);
        if ((dev->lvdtype==lvd_sis1100) && (jdev->addr==16)) {
            struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
            res|=info->mcwrite(dev, 0, ofs(struct lvd_reg, jtag_csr), csr);
            res|=info->mcread(dev, 0, ofs(struct lvd_reg, jtag_csr), &csr);
        } else {
            /* slave controllers not tested */
            res|=lvd_c1_w(dev, jtag_csr, csr);
            res|=lvd_c1_r(dev, jtag_csr, &csr);
        }
        tdo_=!!(csr&LVDMC_JT_TDO);
    } else {
        switch (opaque->mtype) {
        case ZEL_LVD_TDC_F1:
            csr=F1_JT_C|(tms?F1_JT_TMS:0)|(tdi?F1_JT_TDI:0);
            res|=lvd_i_w(dev, jdev->addr, f1.jtag_csr, csr);
            res|=lvd_i_r(dev, jdev->addr, f1.jtag_csr, &csr);
            tdo_=!!(csr&F1_JT_TDO);
            break;
        case ZEL_LVD_ADC_VERTEX:
            csr=VTX_JT_C|(tms?VTX_JT_TMS:0)|(tdi?VTX_JT_TDI:0);
            res|=lvd_i_w(dev, jdev->addr, vertex.jtag_csr, csr);
            res|=lvd_i_r(dev, jdev->addr, vertex.jtag_csr, &csr);
            tdo_=!!(csr&VTX_JT_TDO);
            break;
        case ZEL_LVD_TDC_GPX:
            csr=GPX_JT_C|(tms?GPX_JT_TMS:0)|(tdi?GPX_JT_TDI:0);
            res|=lvd_i_w(dev, jdev->addr, gpx.jtag_csr, csr);
            res|=lvd_i_r(dev, jdev->addr, gpx.jtag_csr, &csr);
            tdo_=!!(csr&GPX_JT_TDO);
            break;
        case ZEL_LVD_SQDC:
        case ZEL_LVD_FQDC:
            csr=QDC_JT_C|(tms?QDC_JT_TMS:0)|(tdi?QDC_JT_TDI:0);
            if (opaque->qdc3) {
                res|=lvd_i_w(dev, jdev->addr, qdc3.jtag_csr, csr);
                res|=lvd_i_r(dev, jdev->addr, qdc3.jtag_csr, &csr);
            } else {
                res|=lvd_i_w(dev, jdev->addr, qdc1.jtag_csr, csr);
                res|=lvd_i_r(dev, jdev->addr, qdc1.jtag_csr, &csr);
            }
            tdo_=!!(csr&QDC_JT_TDO);
            break;
        case ZEL_LVD_MSYNCH:
            csr=MSYNC_JT_C|(tms?MSYNC_JT_TMS:0)|(tdi?MSYNC_JT_TDI:0);
            res|=lvd_i_w(dev, jdev->addr, msync.jtag_csr, csr);
            res|=lvd_i_r(dev, jdev->addr, msync.jtag_csr, &csr);
            tdo_=!!(csr&MSYNC_JT_TDO);
            break;
        case ZEL_LVD_OSYNCH:
            csr=OSYNC_JT_C|(tms?OSYNC_JT_TMS:0)|(tdi?OSYNC_JT_TDI:0);
            res|=lvd_i_w(dev, jdev->addr, osync.jtag_csr, csr);
            res|=lvd_i_r(dev, jdev->addr, osync.jtag_csr, &csr);
            tdo_=!!(csr&OSYNC_JT_TDO);
            break;
        }
    }
    if (tdo!=0)
        *tdo=tdo_;
    return res;    
}
/****************************************************************************/
static int
jt_data(struct jtag_dev* jdev, ems_u32* v)
{
    struct opaque_data* opaque=(struct opaque_data*)jdev->opaque;
    struct lvd_dev* dev=(struct lvd_dev*)jdev->dev;
    int res=0;

    if (jdev->ci) {
        if ((dev->lvdtype==lvd_sis1100) && (jdev->addr==16)) {
            struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
            res=info->mcread(dev, 0, ofs(struct lvd_reg, jtag_data), v)!=plOK;
        } else {
            res=lvd_c1_r(dev, jtag_data, v);
        }
    } else {
        switch (opaque->mtype) {
        case ZEL_LVD_TDC_F1:
            res=lvd_i_r(dev, jdev->addr, f1.jtag_data, v);
            break;
        case ZEL_LVD_ADC_VERTEX:
            res=lvd_i_r(dev, jdev->addr, vertex.jtag_data, v);
            break;
        case ZEL_LVD_TDC_GPX:
            res=lvd_i_r(dev, jdev->addr, gpx.jtag_data, v);
            break;
        case ZEL_LVD_SQDC:
        case ZEL_LVD_FQDC:
            if (opaque->qdc3)
                res=lvd_i_r(dev, jdev->addr, qdc3.jtag_data, v);
            else
                res=lvd_i_r(dev, jdev->addr, qdc1.jtag_data, v);
            break;
        case ZEL_LVD_MSYNCH:
            res=lvd_i_r(dev, jdev->addr, msync.jtag_data, v);
        case ZEL_LVD_OSYNCH:
            res=lvd_i_r(dev, jdev->addr, osync.jtag_data, v);
            break;
        }
    }
    return res;
}
/****************************************************************************/
static int
jt_action_sis1100(struct jtag_dev* jdev, int tms, int tdi, int* tdo)
{
    struct opaque_data_sis1100* opaque=
            (struct opaque_data_sis1100*)jdev->opaque;
    ems_u32 csr, tdo_;

    csr=SIS1100_JT_C|(tms?SIS1100_JT_TMS:0)|(tdi?SIS1100_JT_TDI:0);

    if (ioctl(opaque->p, SIS1100_JTAG_PUT, &csr))
        return -1;
    if (ioctl(opaque->p, SIS1100_JTAG_GET, &csr))
        return -1;
    tdo_=!!(csr&SIS1100_JT_TDO);

    if (tdo!=0)
        *tdo=tdo_;
    return 0;
}
/****************************************************************************/
static int
jt_data_sis1100(struct jtag_dev* jdev, ems_u32* v)
{
    struct opaque_data_sis1100* opaque=
            (struct opaque_data_sis1100*)jdev->opaque;
    int res;
    res=ioctl(opaque->p, SIS1100_JTAG_DATA, v);
    return res;
}
/****************************************************************************/
static void
jt_free(struct jtag_dev* jdev)
{
    free(jdev->opaque);
    jdev->opaque=0;
}
/****************************************************************************/
#ifdef LVD_SIS1100
static int
jt_sleep_sis1100X(struct jtag_dev* jdev)
{
    struct lvd_dev* dev=(struct lvd_dev*)jdev->dev;
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    ems_u32 val;
#   ifdef LVD_SIS1100_MMAPPED
    struct sis1100_reg *pci_ctrl_base=info->A->pci_ctrl_base;
    val=pci_ctrl_base->opt_csr;
#   else
    info->sisread(dev, 0, ofs(struct sis1100_reg, opt_csr), &val);
#   endif
    return val;
}
#endif
static int
jt_sleep(struct jtag_dev* jdev)
{
    struct lvd_dev* dev=(struct lvd_dev*)jdev->dev;
    ems_u32 val;
    lvd_cc_r(dev, ident, &val);
    return val;
}
/****************************************************************************/
#ifdef LVD_SIS1100
static void
jt_mark_sis1100(struct jtag_dev* jdev, int pat)
{
    struct lvd_dev* dev=(struct lvd_dev*)jdev->dev;
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
#ifdef LVD_SIS1100_MMAPPED
    struct sis1100_reg *pci_ctrl_base=info->A->pci_ctrl_base;
    pci_ctrl_base->opt_csr=(pat&0xf)<<4;
#else
    info->siswrite(dev, 0, ofs(struct sis1100_reg, opt_csr), (pat&0xf)<<4);
#endif
}
#endif
static void
jt_mark(struct jtag_dev* jdev, int pat)
{
    struct lvd_dev* dev=(struct lvd_dev*)jdev->dev;
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    info->mcwrite(dev, 0, ofs(struct lvd_reg, cr), (pat&0x3)<<3);
}
/****************************************************************************/
plerrcode
lvd_init_jtag_dev(struct generic_dev* gdev, void* jdev_)
{
    struct lvd_dev* dev=(struct lvd_dev*)gdev;
    struct jtag_dev* jdev=(struct jtag_dev*)jdev_;
    void* opaque=0;
    plerrcode pres=plOK;

    jdev->opaque=0;
    jdev->jt_data=0;
    jdev->jt_action=0;
    jdev->jt_free=jt_free;
    jdev->jt_sleep=0;
    jdev->jt_mark=0;

    switch (jdev->ci) {
    case 0: { /* acquisition card */
        struct opaque_data* opq;
        int i;
        i=lvd_find_acard(dev, jdev->addr);
        if (i<0) {
            printf("init_jdev: illegal card address %d\n", jdev->addr);
            pres=plErr_ArgRange;
            goto error;
        }
        jdev->jt_data=jt_data;
        jdev->jt_action=jt_action;
        jdev->jt_free=jt_free;
#ifdef LVD_SIS1100
        if (dev->lvdtype==lvd_sis1100)
            jdev->jt_sleep=jt_sleep_sis1100X;
        else
#endif
            jdev->jt_sleep=jt_sleep;
        jdev->jt_mark=jt_mark;
        opaque=malloc(sizeof(struct opaque_data));
        if (opaque==0) {
            printf("lowlevel/lvd/lvd_jtag.c:init_jtag_dev: %s\n", strerror(errno));
            return plErr_System;
        }
        opq=(struct opaque_data*)opaque;
        opq->ident=dev->acards[i].id;
        opq->mtype=dev->acards[i].mtype;
        opq->idx=i;
        switch (opq->mtype) {
        case ZEL_LVD_TDC_F1:     break;
        case ZEL_LVD_ADC_VERTEX: break;
        case ZEL_LVD_TDC_GPX:    break;
        case ZEL_LVD_SQDC:       /* no break */
        case ZEL_LVD_FQDC:
            opq->qdc3=LVD_FWver(dev->acards[i].id)>=0x30;
            break;
        case ZEL_LVD_MSYNCH:     break;
        case ZEL_LVD_OSYNCH:     break;
        default:
            printf("init_jdev: unknown card type 0x%x\n", opq->mtype);
            pres=plErr_ArgRange;
            goto error;
        }
      } break;
    case 1: { /* crate controller */
        struct opaque_data* opq;
        if ((unsigned int)jdev->addr>16) {
            printf("init_jdev: illegal controller address %d\n", jdev->addr);
            pres=plErr_ArgRange;
            goto error;
        }
        jdev->jt_data=jt_data;
        jdev->jt_action=jt_action;
        jdev->jt_free=jt_free;
#ifdef LVD_SIS1100
        if (dev->lvdtype==lvd_sis1100)
            jdev->jt_sleep=jt_sleep_sis1100X;
        else
#endif
            jdev->jt_sleep=jt_sleep;
        jdev->jt_mark=jt_mark;
        opaque=malloc(sizeof(struct opaque_data));
        if (opaque==0) {
            printf("lowlevel/lvd/lvd_jtag.c:init_jtag_dev: %s\n",
                strerror(errno));
            return plErr_System;
        }
        opq=(struct opaque_data*)opaque;
        opq->ident=dev->ccard.id;
        opq->mtype=dev->ccard.mtype;
        switch (opq->mtype) {
        case ZEL_LVD_CONTROLLER_GPX: break;
        case ZEL_LVD_CONTROLLER_F1: break;
        default:
            printf("init_jdev: unknown controller type 0x%x\n", opq->mtype);
            pres=plErr_ArgRange;
            goto error;
        }
      } break;
#ifdef LVD_SIS1100
    case 2: { /* sis1100 */
        struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
        if (dev->lvdtype!=lvd_sis1100) {
            printf("init_jdev: only sis1100 as local controller supported\n");
            pres=plErr_BadModTyp;
            goto error;
        }
        if (jdev->addr!=0) {
            printf("init_jdev: illegal sis1100 address %d\n", jdev->addr);
            pres=plErr_ArgRange;
            goto error;
        }
        jdev->jt_data=jt_data_sis1100;
        jdev->jt_action=jt_action_sis1100;
        jdev->jt_free=jt_free;
        jdev->jt_sleep=jt_sleep_sis1100X;
        jdev->jt_mark=jt_mark_sis1100;
        opaque=malloc(sizeof(struct opaque_data_sis1100));
        if (opaque==0) {
            printf("lowlevel/lvd/lvd_jtag.c:init_jtag_dev: %s\n",
                strerror(errno));
            return plErr_System;
        }
        ((struct opaque_data_sis1100*)opaque)->p=info->A->p_ctrl;
      } break;
#endif
    default:
        printf("init_jdev: invalid CIcode %d\n", jdev->ci);
        pres=plErr_ArgRange;
        goto error;
    }
    jdev->opaque=opaque;
    return plOK;

error:
    free(opaque);
    jdev->opaque=0;
    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
