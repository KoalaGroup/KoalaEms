/*
 * procs/lvd/qdc/qdc.c
 * created 2006-Apr-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: qdc.c,v 1.17 2011/11/22 02:00:49 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/qdc/qdc.h"
#include "../../../lowlevel/devices.h"
#include "../lvd_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/lvd/qdc")

/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: module addr  (absolute address or -1 for all modules)
 * p[3]: daqmode      (see manual)
 */
plerrcode proc_lvd_qdc_init(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);

    return lvd_qdc_init(dev, p[2], p[3]);
}

plerrcode test_proc_lvd_qdc_init(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_qdc_init[] = "lvd_qdc_init";
int ver_proc_lvd_qdc_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (4)
 * p[1]: branch
 * p[2]: addr
 * p[3]: latency/ns (if raw==0)
 * p[4]: width/ns   (if raw==0)
 * [p[5]: raw]
 * raw==0 is not allowed any more!
 *
 * window begins 'latency' before trigger and is 'width' wide
 * hits after the trigger have a timestamp <0
 */
plerrcode proc_lvd_qdc_window(ems_u32* p)
{
    ems_i32* ip=(ems_i32*)p;
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_qdc_set_sw_start(dev, (ems_i32)p[2], ip[3]);
    if (pres)
        return pres;
    pres=lvd_qdc_set_sw_length(dev, (ems_i32)p[2], ip[4]);
    return pres;
}

plerrcode test_proc_lvd_qdc_window(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=4) /*&& (p[0]!=5)*/)
        return plErr_ArgNum;
    if (p[0]>4 && !p[5]) {
        printf("lvd_qdc_window: latency and width can only be given in samples\n");
        return plErr_ArgRange;
    }

    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_qdc_window[] = "lvd_qdc_window";
int ver_proc_lvd_qdc_window = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: branch
 * p[2]: addr
 */
plerrcode proc_lvd_qdc_getwindow(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    pres=lvd_qdc_get_sw_start(dev, ip[2], outptr);
    if (pres)
        return pres;
    else
        outptr+=ip[2]<0?16:1;
    pres=lvd_qdc_get_sw_length(dev, ip[2], outptr);
    if (pres)
        return pres;
    else
        outptr+=ip[2]<0?16:1;
    return pres;
}

plerrcode test_proc_lvd_qdc_getwindow(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=32;
    return plOK;
}

char name_proc_lvd_qdc_getwindow[] = "lvd_qdc_getwindow";
int ver_proc_lvd_qdc_getwindow = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: swstart]
 */
plerrcode proc_lvd_qdc_swstart(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_sw_start(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_sw_start(dev, ip[2], outptr);
        if (pres!=plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_swstart(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_swstart[] = "lvd_qdc_swstart";
int ver_proc_lvd_qdc_swstart = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: swlength]
 */
plerrcode proc_lvd_qdc_swlength(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_sw_length(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_sw_length(dev, ip[2], outptr);
        if (pres!=plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_swlength(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_swlength[] = "lvd_qdc_swlength";
int ver_proc_lvd_qdc_swlength = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: intlength]
 */
plerrcode proc_lvd_qdc_intlength(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_sw_ilength(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_sw_ilength(dev, ip[2], outptr);
        if (pres!=plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_intlength(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_intlength[] = "lvd_qdc_intlength";
int ver_proc_lvd_qdc_intlength = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: iwstart]
 */
plerrcode proc_lvd_qdc_iwstart(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_iw_start(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_iw_start(dev, ip[2], outptr);
        if (pres!=plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_iwstart(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_iwstart[] = "lvd_qdc_iwstart";
int ver_proc_lvd_qdc_iwstart = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: iwlength]
 */
plerrcode proc_lvd_qdc_iwlength(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_iw_length(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_iw_length(dev, ip[2], outptr);
        if (pres!=plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_iwlength(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_iwlength[] = "lvd_qdc_iwlength";
int ver_proc_lvd_qdc_iwlength = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: rawcycle]
 */
plerrcode proc_lvd_qdc_rawcycle(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_rawcycle(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_rawcycle(dev, ip[2], outptr);
        if (pres!=plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_rawcycle(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_rawcycle[] = "lvd_qdc_rawcycle";
int ver_proc_lvd_qdc_rawcycle = 1;
/*****************************************************************************/
/*
 * SQDC only
 * p[0]: argcount==2 or 4
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * [p[3]: channel   (0..15 or -1 for all channels)
 *  p[4]: dac       (0..4095)]
 */
plerrcode proc_lvd_qdc_dac(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<4) {
        pres=lvd_qdc_get_dac(dev, p[2], outptr);
        if (pres==plOK)
            outptr+=16;
    } else {
        pres=lvd_qdc_set_dac(dev, ip[2], ip[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_dac(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    if (p[0]>2 && ip[3]>15) {
        printf("lvd_qdc_dac: channel %d not valid\n", ip[3]);
        return plErr_ArgRange;
    }

    if (p[0]>2 && p[4]>4095) {
        printf("lvd_qdc_dac: dac %d not valid\n", p[4]);
        return plErr_ArgRange;
    }

    wirbrauchen=p[0]<4?16:0;
    return plOK;
}

char name_proc_lvd_qdc_dac[] = "lvd_qdc_dac";
int ver_proc_lvd_qdc_dac = 1;
/*****************************************************************************/
/*
 * FQDC only
 * p[0]: argcount==2 or 3
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 *  p[3]: dac       (0..255)]
 */
plerrcode proc_lvd_qdc_testdac(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<3) {
        pres=lvd_qdc_get_testdac(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
    } else {
        pres=lvd_qdc_set_testdac(dev, ip[2], p[3]);
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_testdac(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    if (p[0]>2 && p[3]>255) {
        printf("lvd_qdc_testdac: dac %d not valid\n", p[4]);
        return plErr_ArgRange;
    }

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_testdac[] = "lvd_qdc_testdac";
int ver_proc_lvd_qdc_testdac = 1;
/*****************************************************************************/
/*
 * test_pulse lets some QDCs generate a test pulse.
 * If more than one module is selected the test pulses are not generated
 * at the same time.
 * It is possible to generate a test pulse in nearly all modules
 * (FQDCs, newer SQDCs and all TDCs) at the same time using a broadcast.
 * (see proc_lvd_test_pulse in lvd_init.c)
 *
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr (-1 for all modules)
 */
plerrcode proc_lvd_qdc_test_pulse(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    pres=lvd_qdc_test_pulse(dev, ip[2]);

    return pres;
}

plerrcode test_proc_lvd_qdc_test_pulse(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_qdc_test_pulse[] = "lvd_qdc_test_pulse";
int ver_proc_lvd_qdc_test_pulse = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2 or 3
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 *  p[3]: level]
 */
plerrcode proc_lvd_qdc_trglevel(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<3) {
        pres=lvd_qdc_get_trglevel(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
    } else {
        pres=lvd_qdc_set_trglevel(dev, ip[2], p[3]);
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_trglevel(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_trglevel[] = "lvd_qdc_trglevel";
int ver_proc_lvd_qdc_trglevel = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * [p[3]: channel   (0..15 or -1 for all channels)
 *  p[4]: pedestal]
 */
plerrcode proc_lvd_qdc_pedestal(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<4) {
        pres=lvd_qdc_get_ped(dev, p[2], outptr);
        if (pres==plOK)
            outptr+=16;
    } else {
        pres=lvd_qdc_set_ped(dev, ip[2], ip[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_pedestal(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    if (p[0]>2 && ip[3]>15) {
        printf("lvd_qdc_dac: channel %d not valid\n", ip[3]);
        return plErr_ArgRange;
    }

    wirbrauchen=p[0]<4?16:0;
    return plOK;
}

char name_proc_lvd_qdc_pedestal[] = "lvd_qdc_pedestal";
int ver_proc_lvd_qdc_pedestal = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * [p[3]: channel   (0..15 or -1 for all channels)
 *  p[4]: threshold]
 */
plerrcode proc_lvd_qdc_threshold(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<4) {
        pres=lvd_qdc_get_threshold(dev, p[2], outptr);
        if (pres==plOK)
            outptr+=16;
    } else {
        pres=lvd_qdc_set_threshold(dev, ip[2], ip[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_threshold(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    if (p[0]>2 && ip[3]>15) {
        printf("lvd_qdc_threshold: channel %d not valid\n", ip[3]);
        return plErr_ArgRange;
    }

    wirbrauchen=p[0]<4?16:0;
    return plOK;
}

char name_proc_lvd_qdc_threshold[] = "lvd_qdc_threshold";
int ver_proc_lvd_qdc_threshold = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: module addr (absolute address or -1 for all modules)
 * [p[3]: channel   (0..15 or -1 for all channels)
 *  p[4]: threshold]
 */
plerrcode proc_lvd_qdc_thlevel(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<4) {
        pres=lvd_qdc_get_thrh(dev, p[2], outptr);
        if (pres==plOK)
            outptr+=16;
    } else {
        pres=lvd_qdc_set_thrh(dev, ip[2], ip[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_thlevel(ems_u32* p)
{
    struct lvd_dev* dev;
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    if (p[0]>2 && ip[3]>15) {
        printf("lvd_qdc_thlevel: channel %d not valid\n", ip[3]);
        return plErr_ArgRange;
    }

    wirbrauchen=p[0]<4?16:0;
    return plOK;
}

char name_proc_lvd_qdc_thlevel[] = "lvd_qdc_thlevel";
int ver_proc_lvd_qdc_thlevel = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: mask]
 */
plerrcode proc_lvd_qdc_inhibit(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_inhibit(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_inhibit(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_inhibit(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_inhibit[] = "lvd_qdc_inhibit";
int ver_proc_lvd_qdc_inhibit = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: mask]
 */
plerrcode proc_lvd_qdc_raw(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_raw(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_raw(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_raw(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_raw[] = "lvd_qdc_raw";
int ver_proc_lvd_qdc_raw = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: anal_ctrl]
 */
plerrcode proc_lvd_qdc_anal(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_anal(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_anal(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_anal(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_anal[] = "lvd_qdc_anal";
int ver_proc_lvd_qdc_anal = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_lvd_qdc_noise(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    pres=lvd_qdc_noise(dev, ip[2], outptr);
    if (pres==plOK)
        outptr+=16;

    return pres;
}

plerrcode test_proc_lvd_qdc_noise(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=16;
    return plOK;
}

char name_proc_lvd_qdc_noise[] = "lvd_qdc_noise";
int ver_proc_lvd_qdc_noise = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_lvd_qdc_mean(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    pres=lvd_qdc_mean(dev, ip[2], outptr);
    if (pres==plOK)
        outptr+=16;

    return pres;
}

plerrcode test_proc_lvd_qdc_mean(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=16;
    return plOK;
}

char name_proc_lvd_qdc_mean[] = "lvd_qdc_mean";
int ver_proc_lvd_qdc_mean = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr (-1 for all modules)
 */
plerrcode proc_lvd_qdc_ovr(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    pres=lvd_qdc_ovr(dev, ip[2], outptr);
    if (pres==plOK)
        outptr+=ip[2]<0?16:1;

    return pres;
}

plerrcode test_proc_lvd_qdc_ovr(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=16;
    return plOK;
}

char name_proc_lvd_qdc_ovr[] = "lvd_qdc_ovr";
int ver_proc_lvd_qdc_ovr = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: module addr (-1 for all modules)
 */
plerrcode proc_lvd_qdc_bline_adjust(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    pres=lvd_qdc_bline_adjust(dev, ip[2], outptr);
    if (pres==plOK)
        outptr+=ip[2]<0?16:1;

    return pres;
}

plerrcode test_proc_lvd_qdc_bline_adjust(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=16;
    return plOK;
}

char name_proc_lvd_qdc_bline_adjust[] = "lvd_qdc_bline_adjust";
int ver_proc_lvd_qdc_bline_adjust = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: grp_coinc pattern]
 */
plerrcode proc_lvd_qdc_grp_coinc(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_grp_coinc(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_grp_coinc(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_grp_coinc(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_grp_coinc[] = "lvd_qdc_grp_coinc";
int ver_proc_lvd_qdc_grp_coinc = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 5)
 * p[1]: branch
 * p[2]: module addr (0..16 or -1 for all modules of appropriate type)
 * [p[3]: FPGA idx (0..3 or -1 for all four groups)
 *  p[4]: coincidence lookup pattern]
 */
plerrcode proc_lvd_qdc_coinc(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_coinc(dev, ip[2], ip[3], p[4]);
    } else {
        pres=lvd_qdc_get_coinc(dev, ip[2], ip[3], outptr);
        if (pres==plOK)
            outptr+=(ip[2]<0?16:1)*(ip[3]<0?4:1);
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_coinc(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=5))
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?64:0;
    return plOK;
}

char name_proc_lvd_qdc_coinc[] = "lvd_qdc_coinc";
int ver_proc_lvd_qdc_coinc = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 3)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: sc_rate (0: no readout; else: every 2**(n-1) events; max. n=15)
 */
plerrcode proc_lvd_qdc_sc_rate(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]>2) {
        pres=lvd_qdc_set_sc_rate(dev, ip[2], p[3]);
    } else {
        pres=lvd_qdc_get_sc_rate(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
    }
    return pres;
}

plerrcode test_proc_lvd_qdc_sc_rate(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if (p[0]>2 && p[3]>15)
        return plErr_ArgRange;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=p[0]<3?16:0;
    return plOK;
}

char name_proc_lvd_qdc_sc_rate[] = "lvd_qdc_sc_rate";
int ver_proc_lvd_qdc_sc_rate = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (2 or 4)
 * p[1]: branch
 * p[2]: module addr
 * [p[3]: first channel
 *  p[4]: last channel]
 * (disable if last<first)
 */
plerrcode proc_lvd_qdc_sc_init(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]<3)
        pres=lvd_qdc_sc_init(dev, ip[2], 0, 15);
    else
        pres=lvd_qdc_sc_init(dev, ip[2], p[3], p[4]);
    return pres;
}

plerrcode test_proc_lvd_qdc_sc_init(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2 && p[0]!=4)
        return plErr_ArgNum;
    if (p[0]>2) {
	if (p[3]>15 || p[4]>15)
	    return plErr_ArgRange;
    }
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_qdc_sc_init[] = "lvd_qdc_sc_init";
int ver_proc_lvd_qdc_sc_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (1 or more)
 * p[1]: branch
 * p[2]: module addr
 */
plerrcode proc_lvd_qdc_sc_clear(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres=3;
    ems_i32* ip=(ems_i32*)p;

    pres=lvd_qdc_sc_clear(dev, ip[2]);
    return pres;
}

plerrcode test_proc_lvd_qdc_sc_clear(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_qdc_sc_clear[] = "lvd_qdc_sc_clear";
int ver_proc_lvd_qdc_sc_clear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (==1)
 * p[1]: branch
 */
plerrcode proc_lvd_qdc_sc_update(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    plerrcode pres;

    pres=lvd_qdc_sc_update(dev);
    return pres;
}

plerrcode test_proc_lvd_qdc_sc_update(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_qdc_sc_update[] = "lvd_qdc_sc_update";
int ver_proc_lvd_qdc_sc_update = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (3)
 * p[1]: branch
 * p[2]: module addr
 * p[3]: mask
 */
plerrcode proc_lvd_qdc_sc_get(ems_u32* p)
{
    struct lvd_dev* dev=get_lvd_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    pres=lvd_qdc_sc_get(dev, ip[2], p[3], outptr, &num);
    if (pres==plOK)
        outptr+=num;

    return pres;
}

plerrcode test_proc_lvd_qdc_sc_get(ems_u32* p)
{
    struct lvd_dev* dev;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (p[3]&0xffff0000)
	return plErr_ArgRange;
    if ((pres=find_lvd_odevice(p[1], &dev))!=plOK)
        return pres;

    /* 16 64-bit values and two header words per module */
    wirbrauchen=34*(p[2]<0?15:1);
    return plOK;
}

char name_proc_lvd_qdc_sc_get[] = "lvd_qdc_sc_get";
int ver_proc_lvd_qdc_sc_get = 1;
/*****************************************************************************/
/*****************************************************************************/
