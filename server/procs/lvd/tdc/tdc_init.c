/*
 * procs/lvd/tdc/tdc_init.c
 * created 2006-05-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: tdc_init.c,v 1.6 2017/10/20 23:19:11 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/tdc/tdc.h"
#include "../../../lowlevel/devices.h"
#include "../lvd_verify.h"

#define get_device(branch) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (branch))

static ems_u32 mtypes[]={ZEL_LVD_TDC_F1, ZEL_LVD_TDC_GPX, 0};

RCS_REGISTER(cvsid, "procs/lvd/tdc")

/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: module addr  (absolute address or -1 for all modules)
 * p[3]: chip           (0..7 or (8 or <0) for all chips)
 * p[4]: raising edge (0|1)
 * p[5]: falling edge (0|1)
 * p[6]: daqmode      (see manual)
 */
plerrcode proc_lvd_tdc_init(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int edges;

    edges=(p[4]?1:0)|(p[5]?2:0);
    return lvd_tdc_init(dev, p[2], p[3], edges, p[6]);
}

plerrcode test_proc_lvd_tdc_init(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=6)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK)
            return pres;
    }

    if (ip[3]>8) {
        printf("lvd_tdc_init: chip %d not valid\n", ip[3]);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_tdc_init[] = "lvd_tdc_init";
int ver_proc_lvd_tdc_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (4 or 5)
 * p[1]: branch
 * p[2]: addr
 * p[3]: latency/ns (if raw==0)
 * p[4]: width/ns   (if raw==0)
 * [p[5]: raw]
 *
 * window begins 'latency' before trigger and is 'width' wide
 * hits after the trigger have a timestamp <0
 */
plerrcode proc_lvd_tdc_window(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int raw;
    raw=p[0]<5?0:p[5];

    return lvd_tdc_window(dev, (ems_i32)p[2], p[3], p[4], raw);
}

plerrcode test_proc_lvd_tdc_window(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=4) && (p[0]!=5))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_tdc_window[] = "lvd_tdc_window";
int ver_proc_lvd_tdc_window = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: addr
 * p[3]: raw
 */
plerrcode proc_lvd_tdc_getwindow(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int raw;
    raw=p[0]<5?0:p[5];

    return lvd_tdc_getwindow(dev, p[2], outptr+0, outptr+1, raw);
}

plerrcode test_proc_lvd_tdc_getwindow(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=2;
    return plOK;
}

char name_proc_lvd_tdc_getwindow[] = "lvd_tdc_getwindow";
int ver_proc_lvd_tdc_getwindow = 1;
/*****************************************************************************/
/*
 * p[0]: argcount == 4 or 5
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: connector   (0..3 or -1 for all connectors)
 * p[4]: dac         (0..255)
 *
 * p[2..4]<0: all
 *
 * version 1: front connectors
 * version 2: rear connectors
 */
static plerrcode
proc_lvd_tdc_dac_X(ems_u32* p, int version)
{
    struct lvd_dev* dev=get_device(p[1]);
    ems_i32* ip=(ems_i32*)p;

    return lvd_tdc_dac(dev, ip[2], ip[3], p[4], version);
}
plerrcode proc_lvd_tdc_dac_1(ems_u32* p)
{
    return proc_lvd_tdc_dac_X(p, 1);
}
plerrcode proc_lvd_tdc_dac_2(ems_u32* p)
{
    return proc_lvd_tdc_dac_X(p, 2);
}

static plerrcode
test_proc_lvd_tdc_dac_X(ems_u32* p)
{
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;

    if (ip[3]>7) {
        printf("lvd_tdc_dac: connector %d not valid\n", ip[3]);
        return plErr_ArgRange;
    }

    if (p[4]>255) {
        printf("lvd_tdc_dac: dac %d not valid\n", p[4]);
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}
plerrcode test_proc_lvd_tdc_dac_1(ems_u32* p)
{
    return test_proc_lvd_tdc_dac_X(p);
}
plerrcode test_proc_lvd_tdc_dac_2(ems_u32* p)
{
    return test_proc_lvd_tdc_dac_X(p);
}
char name_proc_lvd_tdc_dac_1[] = "lvd_tdc_dac";
int ver_proc_lvd_tdc_dac_1 = 1;
char name_proc_lvd_tdc_dac_2[] = "lvd_tdc_dac";
int ver_proc_lvd_tdc_dac_2 = 2;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: addr (<0: crate controller)
 * p[3]: chip (0..7)
 */
plerrcode proc_hsdiv_search(ems_u32* p)
{
    int min, max;
    plerrcode pres;

    struct lvd_dev* dev=get_device(p[1]);

    if ((pres=hsdiv_search(dev, p[2], p[3], &min, &max, 1))!=plOK)
        return pres;

    *outptr++=min;
    *outptr++=max;
    return plOK;
}

plerrcode test_proc_hsdiv_search(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[3]>7)
        return plErr_ArgRange;
    wirbrauchen=2;
    return plOK;
}

char name_proc_hsdiv_search[] = "hsdiv_search";
int ver_proc_hsdiv_search = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: addr (<0: crate controller)
 * p[3]: chip (0..7)
 */
plerrcode proc_hsdiv_search2(ems_u32* p)
{
    int min, max;
    plerrcode pres;

    struct lvd_dev* dev=get_device(p[1]);

    if ((pres=hsdiv_search(dev, p[2], p[3], &min, &max, 2))!=plOK)
        return pres;

    *outptr++=min;
    *outptr++=max;
    return plOK;
}

plerrcode test_proc_hsdiv_search2(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[3]>7)
        return plErr_ArgRange;
    wirbrauchen=2;
    return plOK;
}

char name_proc_hsdiv_search2[] = "hsdiv_search";
int ver_proc_hsdiv_search2 = 2;
/*****************************************************************************/
/*
 * p[0]: argcount (4 or 5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all LVD TDC modules)
 * p[3]: channel     (idx (0..63) or -1 for all channels)
 * p[4]: edge mask   (0: none 1: raising 2: falling 3: both)
 * [p[5]: immediate]
 *
 * p[2..4]<0: all
 */
plerrcode proc_lvd_tdc_edge(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    ems_i32* ip=(ems_i32*)p;

    return lvd_tdc_edge(dev, ip[2], ip[3], p[4], p[0]>4?p[5]:0);
}

plerrcode test_proc_lvd_tdc_edge(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if ((p[0]!=4) && (p[0]!=5))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK)
            return pres;
    }

    if (ip[3]>63) {
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_tdc_edge[] = "lvd_tdc_edge";
int ver_proc_lvd_tdc_edge = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (4 or 5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * p[3]: channel mask1 (channels 32..63)
 * p[4]: channel mask0 (channels 0..31)
 * p[5]: edge mask   (0: none 1: raising 2: falling 3: both)
 * [p[6]: immediate]
 *
 * p[2]<0: all
 */
plerrcode proc_lvd_tdc_edge2(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres=plOK;
    ems_i32* ip=(ems_i32*)p;

    if ((p[3]==p[4]) && (p[3]==0xffffffff)) {
        pres=lvd_tdc_edge(dev, ip[2], -1, p[5], p[0]>5?p[6]:0);
    } else {
        int i;
        for (i=0; i<32; i++) {
            if ((1<<i)&p[3]) {
                pres=lvd_tdc_edge(dev, ip[2], i+32, p[5], p[0]>5?p[6]:0);
                if (pres!=plOK)
                    break;
            }
        }
        for (i=0; i<32; i++) {
            if ((1<<i)&p[4]) {
                pres=lvd_tdc_edge(dev, ip[2], i, p[5], p[0]>5?p[6]:0);
                if (pres!=plOK)
                    break;
            }
        }
    }
    return pres;
}

plerrcode test_proc_lvd_tdc_edge2(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if ((p[0]!=5) && (p[0]!=6))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_tdc_edge2[] = "lvd_tdc_edge";
int ver_proc_lvd_tdc_edge2 = 2;
/*****************************************************************************/
/*
 * p[0]: argcount (4 or 5)
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all LVD TDC modules)
 * [p[3]: high mask (for channels 0..31)
 *  p[4]: low  mask (for channels 32..63)]
 */
plerrcode proc_lvd_tdc_mask(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]==0) {
        pres=lvd_tdc_get_mask(dev, ip[2], outptr);
        if (pres==plOK)
            outptr++;
    } else {
        pres=lvd_tdc_mask(dev, ip[2], p+3);
    }
    return pres;
}

plerrcode test_proc_lvd_tdc_mask(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if ((p[0]!=2) || (p[0]!=4))
        return plErr_ArgNum;
    if (p[0]==2 && ip[2]<0)
        return plErr_ArgRange;

    if ((pres=find_odevice(modul_lvd, p[1], (struct generic_dev**)&dev))!=plOK)
        return pres;

    if (ip[2]>=0) {
        pres=verify_lvd_ids(dev, p[2], mtypes);
        if (pres!=plOK)
            return pres;
    }

    wirbrauchen=p[0]==2?2:0;
    return plOK;
}

char name_proc_lvd_tdc_mask[] = "lvd_tdc_mask";
int ver_proc_lvd_tdc_mask = 1;
/*****************************************************************************/
/*****************************************************************************/
