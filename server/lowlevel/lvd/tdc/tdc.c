/*
 * lowlevel/lvd/tdc/tdc.c
 * created 2006-May-08 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: tdc.c,v 1.7 2013/01/17 22:44:55 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../commu/commu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <modultypes.h>
#include <rcs_ids.h>
#include "f1.h"
#include "gpx.h"
#include "tdc.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/tdc")

/*****************************************************************************/
static plerrcode
lvd_tdc_init_(struct lvd_dev* dev, struct lvd_acard* acard, int chip,
        int edges, int mode)
{
    plerrcode pres=plOK;

    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        pres=f1_init_(dev, acard, chip, edges, mode);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_init_(dev, acard, chip, edges, mode);
        break;
    default:
        complain("lvd_tdc_init_: lvd card with address 0x%x is not a TDC",
            acard->addr);
        pres=plErr_Program;
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_tdc_init(struct lvd_dev* dev, int addr, int chip, int edges, int mode)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if ((dev->acards[i].mtype==ZEL_LVD_TDC_F1) ||
                (dev->acards[i].mtype==ZEL_LVD_TDC_GPX)) {
                pres=lvd_tdc_init_(dev, dev->acards+i, chip, edges, mode);
                if (pres!=plOK)
                    break;
            }
        }
    } else {
        struct lvd_acard* acard;
        int idx=lvd_find_acard(dev, addr);
        if (idx<0) {
            complain("lvd_tdc_init: no lvd card with address 0x%x known",
                    addr);
            return plErr_Program;
        }
        acard=dev->acards+idx;
        if ((acard->mtype!=ZEL_LVD_TDC_F1) && (acard->mtype!=ZEL_LVD_TDC_GPX)) {
            complain("lvd_tdc_init: lvd card with address 0x%x is not a TDC\n",
                    addr);
            return plErr_Program;
        }
        pres=lvd_tdc_init_(dev, acard, chip, edges, mode);
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_tdc_edge_(struct lvd_dev* dev, struct lvd_acard* acard, int channel,
        int edges, int immediate)
{
    plerrcode pres=plOK;

    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        pres=f1_edge_(dev, acard, channel, edges, immediate);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_edge_(dev, acard, channel, edges, immediate);
        break;
    default:
        complain("lvd_tdc_edge_: lvd card with address 0x%x is not a TDC",
            acard->addr);
        pres=plErr_Program;
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_tdc_edge(struct lvd_dev* dev, int addr, int channel,
        int edges, int immediate)
{
    plerrcode pres=plOK;

    edges&=3;
    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if ((dev->acards[i].mtype==ZEL_LVD_TDC_F1) ||
                (dev->acards[i].mtype==ZEL_LVD_TDC_GPX)) {
                pres=lvd_tdc_edge_(dev, dev->acards+i,
                        channel, edges, immediate);
                if (pres!=plOK)
                    break;
            }
        }
    } else {
        struct lvd_acard* acard;
        int idx=lvd_find_acard(dev, addr);
        if (idx<0) {
            complain("lvd_tdc_edge: no lvd card with address 0x%x known",
                    addr);
            return plErr_Program;
        }
        acard=dev->acards+idx;
        if ((acard->mtype!=ZEL_LVD_TDC_F1) && (acard->mtype!=ZEL_LVD_TDC_GPX)) {
            complain("lvd_tdc_edge: lvd card with address 0x%x is not a TDC",
                    addr);
            return plErr_Program;
        }
        pres=lvd_tdc_edge_(dev, acard, channel, edges, immediate);
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_tdc_dac_(struct lvd_dev* dev, struct lvd_acard* acard, int connector,
        int dac, int version)
{
    plerrcode pres=plOK;

    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        pres=f1_dac_(dev, acard, connector, dac, version);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_dac_(dev, acard, connector, dac, version);
        break;
    default:
        complain("lvd_tdc_dac_: lvd card with address 0x%x is not a TDC",
            acard->addr);
        pres=plErr_Program;
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_tdc_dac(struct lvd_dev* dev, int addr, int connector, int dac, int version)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if ((dev->acards[i].mtype==ZEL_LVD_TDC_F1) ||
                (dev->acards[i].mtype==ZEL_LVD_TDC_GPX)) {
                pres=lvd_tdc_dac_(dev, dev->acards+i,
                        connector, dac, version);
                if (pres!=plOK)
                    break;
            }
        }
    } else {
        struct lvd_acard* acard;
        int idx=lvd_find_acard(dev, addr);
        if (idx<0) {
            complain("lvd_tdc_dac: no lvd card with address 0x%x known",
                    addr);
            return plErr_Program;
        }
        acard=dev->acards+idx;
        if ((acard->mtype!=ZEL_LVD_TDC_F1) && (acard->mtype!=ZEL_LVD_TDC_GPX)) {
            complain("lvd_tdc_dac: lvd card with address 0x%x is not a TDC",
                    addr);
            return plErr_Program;
        }
        pres=lvd_tdc_dac_(dev, acard, connector, dac, version);
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_tdc_window_(struct lvd_dev* dev, struct lvd_acard* acard, int latency,
    int width, int raw)
{
    plerrcode pres=plOK;

    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        pres=f1_window_(dev, acard, latency, width, raw);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_window_(dev, acard, latency, width, raw);
        break;
    default:
        complain("lvd_tdc_window_: lvd card with address 0x%x is not a TDC",
            acard->addr);
        pres=plErr_Program;
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_tdc_window(struct lvd_dev* dev, int addr, int latency, int width, int raw)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if ((dev->acards[i].mtype==ZEL_LVD_TDC_F1) ||
                (dev->acards[i].mtype==ZEL_LVD_TDC_GPX)) {
                pres=lvd_tdc_window_(dev, dev->acards+i, latency, width, raw);
                if (pres!=plOK)
                    break;
            }
        }
    } else {
        struct lvd_acard* acard;
        int idx=lvd_find_acard(dev, addr);
        if (idx<0) {
            complain("lvd_tdc_window: no lvd card with address 0x%x known",
                    addr);
            return plErr_Program;
        }
        acard=dev->acards+idx;
        if ((acard->mtype!=ZEL_LVD_TDC_F1) && (acard->mtype!=ZEL_LVD_TDC_GPX)) {
            complain("lvd_tdc_window: lvd card with address 0x%x is not a TDC",
                    addr);
            return plErr_Program;
        }
        pres=lvd_tdc_window_(dev, acard, latency, width, raw);
    }
    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_tdc_getwindow_(struct lvd_dev* dev, struct lvd_acard* acard,
    ems_u32* latency, ems_u32* width, int raw)
{
    plerrcode pres=plOK;

    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        pres=f1_getwindow_(dev, acard, latency, width, raw);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_getwindow_(dev, acard, latency, width, raw);
        break;
    default:
        complain("lvd_tdc_getwindow_: lvd card with address 0x%x is not a TDC",
            acard->addr);
        pres=plErr_Program;
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_tdc_getwindow(struct lvd_dev* dev, int addr, ems_u32* latency,
    ems_u32* width, int raw)
{
    plerrcode pres=plOK;

    struct lvd_acard* acard;
    int idx=lvd_find_acard(dev, addr);
    if (idx<0) {
        complain("lvd_tdc_getwindow: no lvd card with address 0x%x known",
                addr);
        return plErr_Program;
    }
    acard=dev->acards+idx;
    if ((acard->mtype!=ZEL_LVD_TDC_F1) && (acard->mtype!=ZEL_LVD_TDC_GPX)) {
        complain("lvd_tdc_getwindow: lvd card with address 0x%x is not a TDC",
            addr);
        return plErr_Program;
    }
    pres=lvd_tdc_getwindow_(dev, acard, latency, width, raw);

    return pres;
}
/*****************************************************************************/
plerrcode
hsdiv_search(struct lvd_dev* dev, int addr, int chip, int* min, int* max,
    int version)
{
/* XXX addr<0 (controller) not handled here */
    struct lvd_acard* acard;
    int idx;
    plerrcode pres=plOK;

    idx=lvd_find_acard(dev, addr);
    if (idx<0) {
        complain("hsdiv_search: no lvd card with address 0x%x known", addr);
        return plErr_ArgRange;
    }
    acard=dev->acards+idx;
    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        if (version==1)
            pres=f1_hsdiv_search(dev, addr, chip, min, max);
        else
            pres=f1_hsdiv_search2(dev, addr, chip, min, max);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_hsdiv_search(dev, addr, chip, min, max);
        break;
    default:
        complain("hsdiv_search: lvd card with address 0x%x is not a TDC"
            " (type=0x%08x)\n", addr, acard->mtype);
        return plErr_ArgRange;
    }

    return pres;
}
/*****************************************************************************/
static plerrcode
lvd_tdc_mask_(struct lvd_dev* dev, struct lvd_acard* acard, u_int32_t* mask)
{
    plerrcode pres=plOK;

    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        pres=f1_mask_(dev, acard, mask);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_mask_(dev, acard, mask);
        break;
    default:
        complain("lvd_tdc_mask_: lvd card with address 0x%x is not a TDC",
            acard->addr);
        pres=plErr_Program;
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_tdc_mask(struct lvd_dev* dev, int addr, u_int32_t* mask)
{
    plerrcode pres=plOK;

    if (addr<0) {
        int i;
        for (i=0; i<dev->num_acards; i++) {
            if ((dev->acards[i].mtype==ZEL_LVD_TDC_F1) ||
                (dev->acards[i].mtype==ZEL_LVD_TDC_GPX)) {
                pres=lvd_tdc_mask_(dev, dev->acards+i, mask);
                if (pres!=plOK)
                    break;
            }
        }
    } else {
        struct lvd_acard* acard;
        int idx=lvd_find_acard(dev, addr);
        if (idx<0) {
            complain("lvd_tdc_mask: no lvd card with address 0x%x known",
                    addr);
            return plErr_Program;
        }
        acard=dev->acards+idx;
        if ((acard->mtype!=ZEL_LVD_TDC_F1) && (acard->mtype!=ZEL_LVD_TDC_GPX)) {
            complain("lvd_tdc_mask: lvd card with address 0x%x is not a TDC",
                    addr);
            return plErr_Program;
        }
        pres=lvd_tdc_mask_(dev, acard, mask);
    }
    return pres;
}
/*****************************************************************************/
plerrcode
lvd_tdc_get_mask(struct lvd_dev* dev, int addr, u_int32_t* mask)
{
    plerrcode pres=plOK;
    struct lvd_acard* acard;
    int idx=lvd_find_acard(dev, addr);

    if (idx<0) {
        complain("lvd_tdc_get_mask: no lvd card with address 0x%x known",
                addr);
        return plErr_Program;
    }
    acard=dev->acards+idx;
    if ((acard->mtype!=ZEL_LVD_TDC_F1) && (acard->mtype!=ZEL_LVD_TDC_GPX)) {
        complain("lvd_tdc_get_mask: lvd card with address 0x%x is not a TDC",
                addr);
        return plErr_Program;
    }

    switch (acard->mtype) {
    case ZEL_LVD_TDC_F1:
        pres=f1_get_mask_(dev, acard, mask);
        break;
    case ZEL_LVD_TDC_GPX:
        pres=gpx_get_mask_(dev, acard, mask);
        break;
    default:
        complain("lvd_tdc_mask_: lvd card with address 0x%x is not a TDC",
            acard->addr);
        pres=plErr_Program;
    }

    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
