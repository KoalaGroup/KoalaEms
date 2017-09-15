/*
 * procs/lvd/lvd_init.c
 * created 2005-Feb-23 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_init.c,v 1.39 2013/01/17 22:40:41 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/lvd/lvdbus.h"
#include "../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(branch) \
    ((struct lvd_dev*)get_gendevice(modul_lvd, (branch)))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==1..17
 * p[1]: branch
 * [p[2..17]] predefined addresses 
 */
plerrcode proc_lvdbus_init(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    if ((pres=lvdbus_reset(dev))!=plOK)
        return pres;
    return lvdbus_init(dev, p[0]-1, p+2, 0);
}

plerrcode test_proc_lvdbus_init(ems_u32* p)
{
    plerrcode pres;
    if ((p[0]<1) | (p[0]>17))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvdbus_init[] = "lvdbus_init";
int ver_proc_lvdbus_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: trigger input mask
 *        0x1: LEMO, 0x2: RJ45, 0x4: 6pack, 0x8: optics
 * p[3]: hsdiv (-1: default for mixing F1 and GPX, 0: natural default)
 * p[4]: edge: 1:raising 0:falling
 * [p[5]: mode for f1gpx controller: -1: automatic, 0: F1, 1: GPX
 *
 *  resolution=(T_REF*(1<<REFCLKDIV))/(152.0*hsdiv);
 *      REFCLKDIV=7 (f1_map.h)
 *      T_REF=25 ns (f1_map.h / hardware: 40 MHz Clock)
 *          ==(25ns*128/(152*hsdiv))
 *          ==(21052.63ps/hsdiv)
 */
plerrcode proc_lvd_init(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return lvd_init_controllers(dev, p[2], p[3], p[4], p[0]>4?p[5]:-1);
}

plerrcode test_proc_lvd_init(ems_u32* p)
{
    plerrcode pres;
    if (p[0]<4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_init[] = "lvd_init";
int ver_proc_lvd_init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: branch
 */
plerrcode proc_lvd_eventcount(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=lvd_get_eventcount(dev, outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}

plerrcode test_proc_lvd_eventcount(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_eventcount[] = "lvd_eventcount";
int ver_proc_lvd_eventcount = 1;
/*****************************************************************************/
//old, for write only:
/*
 * p[0]: argcount==2|3
 * p[1]: branch
 * p[2]: controller (-1 for all) only -1 or 16 is valid here
 * p[3]: mode for controller card(s)
 */
//new:
/*
 * p[0]: argcount==1|2
 * p[1]: branch
 * [p[2]: mode for controller card
 */
/*
 * p3:
 * main mode:
 *   0: idle
 *   1: DAQTRG
 *   2: DAQRAW
 *   3: TRIGRAW
 * modifier bits:
 *  0x4: DAQ_TEST     use test pulse for trigger
 *  0x8: DAQ_HANDSH   require handshake for each event
 * 0x10: DAQ_LOCEVNR  generate local event number
 * 0x20: F1_FORMAT    trigger time in F1 format (GPX only)
 * 0x40: NO_LVDSTRT   no automatic synchronisation (hardware test only)
 */

plerrcode proc_lvd_daqmode(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres=plOK;

    switch (p[0]) {
    case 1: /* read (new style) */
        pres=lvd_getdaqmode(dev, outptr);
        if (pres==plOK)
            outptr++;
        break;
    case 2: /* write (new style) */
        pres=lvd_setdaqmode(dev, p[2]);
        break;
    case 3: /* write (old style*/
        pres=lvd_setdaqmode(dev, p[3]);
        break;
    }
    return pres;
}

plerrcode test_proc_lvd_daqmode(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;

    switch (p[0]) {
    case 1: /* read (new style) */
        wirbrauchen=1;
        break;
    case 2: /* write (new style) */
        wirbrauchen=0;
        break;
    case 3: /* write (old style*/
        wirbrauchen=0;
        break;
    default:
        return plErr_ArgNum;
    }
    return plOK;
}

char name_proc_lvd_daqmode[] = "lvd_daqmode";
int ver_proc_lvd_daqmode = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2|4
 * p[1]: branch
 * p[2]: module addr (-1 for all)
 * [[p[3]: module type]
 *  p[3|4]: mode for input card(s)]
 */
plerrcode proc_lvdacq_daqmode(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres=plErr_Program;

    switch (p[0]) {
    case 2:
        pres=lvdacq_getdaqmode(dev, ip[2], outptr);
        if (pres==plOK)
            outptr+=ip[2]<0?16:1;
        break;
    case 3:
        pres=lvdacq_setdaqmode(dev, ip[2], 0, p[3]);
        break;
    case 4:
        pres=lvdacq_setdaqmode(dev, ip[2], p[3], p[4]);
        break;
    }
    return pres;
}

plerrcode test_proc_lvdacq_daqmode(ems_u32* p)
{
    ems_i32* ip=(ems_i32*)p;
    plerrcode pres;

    if (p[0]<1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;

    switch (p[0]) {
    case 2:
        wirbrauchen=ip[2]<0?16:1;
        break;
    case 3:
        if (ip[2]<0)
            return plErr_ArgNum;
    case 4:
        wirbrauchen=0;
        break;
    default:
        return plErr_ArgNum;
    }

    return plOK;
}

char name_proc_lvdacq_daqmode[] = "lvdacq_daqmode";
int ver_proc_lvdacq_daqmode = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: trace (0: disable, 1: enable, -1: only get current value)
 *       return value==-1: settrace not implemented
 */
plerrcode proc_lvd_trace(ems_u32* p)
{
#ifdef LVD_TRACE
    struct lvd_dev* dev=get_device(p[1]);
    int trace;
    lvd_settrace(dev, &trace, p[2]);
    *outptr++=trace;
    printf("lvd(%d): trace %s\n", p[1], p[2]?"on":"off"); 
    return plOK;
#else
    return plErr_NoSuchProc;
#endif
}

plerrcode test_proc_lvd_trace(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_trace[] = "lvd_trace";
int ver_proc_lvd_trace = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: silent (0: verbose, 1: silent, -1: only get current value)
 */
plerrcode proc_lvd_silent_regmanipulation(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    *outptr++=lvd_silent_regmanipulation(dev, p[2]);
    return plOK;
}

plerrcode test_proc_lvd_silent_regmanipulation(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_silent_regmanipulation[] = "lvd_silent_regmanipulation";
int ver_proc_lvd_silent_regmanipulation = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: c/i (1: controller 0: input card)
 * p[3]: addr
 * p[4]: reg
 * p[5]: size (2/4)
 * p[6]: num
 */
plerrcode proc_lvd_blockread(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;
    size_t num=p[6];
    ems_u32* help=outptr++;

    pres=dev->read_block(dev, p[2], p[3], p[4], p[5], outptr, &num);
    if (pres==plOK) {
        outptr+=num;
        *help=num;
    } else {
        outptr--;
    }
    return pres;
}

plerrcode test_proc_lvd_blockread(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=6)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[5]!=2 && p[5]!=4)
        return plErr_ArgRange;
    wirbrauchen=1+(p[6]+3)/4;
    return plOK;
}

char name_proc_lvd_blockread[] = "lvd_blockread";
int ver_proc_lvd_blockread = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==6
 * p[1]: branch
 * p[2]: c/i (1: controller 0: input card)
 * p[3]: addr
 * p[4]: reg
 * p[5]: size (2/4)
 * p[6]: num
 * p[7}... data
 */
plerrcode proc_lvd_blockwrite(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;
    size_t num=p[6];

    pres=dev->write_block(dev, p[2], p[3], p[4], p[5], p+7, &num);
    return pres;
}

plerrcode test_proc_lvd_blockwrite(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<6)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[5]!=2 && p[5]!=4)
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_blockwrite[] = "lvd_blockwrite";
int ver_proc_lvd_blockwrite = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4|5
 * p[1]: branch
 * p[2]: controller
 * p[3]: reg
 * p[4]: size (2|4)
 * [p[5]: val]
 */
plerrcode proc_lvd_creg(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    if (p[0]==4) {
        pres=lvd_get_creg(dev, p[3], p[4], outptr);
        if (pres==plOK)
            outptr++;
    } else {
        pres=lvd_set_creg(dev, p[3], p[4], p[5]);
    }
    return pres;
}

plerrcode test_proc_lvd_creg(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=4) && (p[0]!=5))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]==4?1:0;
    return plOK;
}

char name_proc_lvd_creg[] = "lvd_creg";
int ver_proc_lvd_creg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3|4
 * p[1]: branch
 * p[2]: reg
 * p[3]: size (2|4)
 * [p[4]: val]
 */
plerrcode proc_lvd_creg_2(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    if (p[0]==3) {
        pres=lvd_get_creg(dev, p[2], p[3], outptr);
        if (pres==plOK)
            outptr++;
    } else {
        pres=lvd_set_creg(dev, p[2], p[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_lvd_creg_2(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=3) && (p[0]!=4))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]==3?1:0;
    return plOK;
}

char name_proc_lvd_creg_2[] = "lvd_creg";
int ver_proc_lvd_creg_2 = 2;
/*****************************************************************************/
/*
 * p[0]: argcount==4|5
 * p[1]: branch
 * p[2]: acq-card
 * p[3]: reg
 * p[4]: size (2|4)
 * [p[5]: val]
 */
plerrcode proc_lvd_areg(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    if (p[0]==4) {
        res=lvd_get_areg(dev, p[2], p[3], p[4], outptr);
        if (res==plOK)
            outptr++;
    } else {
        res=lvd_set_areg(dev, p[2], p[3], p[4], p[5]);
    }
    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_areg(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=4) && (p[0]!=5))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]==4?1:0;
    return plOK;
}

char name_proc_lvd_areg[] = "lvd_areg";
int ver_proc_lvd_areg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4|5
 * p[1]: branch
 * p[2]: reg
 * p[3]: size (2|4)
 * [p[4]: val]
 */
plerrcode proc_lvd_aregB(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    if (p[0]==3) {
        res=lvd_get_aregB(dev, p[2], p[3], outptr);
        if (res==plOK)
            outptr++;
    } else {
        res=lvd_set_aregB(dev, p[2], p[3], p[4]);
    }
    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_aregB(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=3) && (p[0]!=4))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]==3?1:0;
    return plOK;
}

char name_proc_lvd_aregB[] = "lvd_aregB";
int ver_proc_lvd_aregB = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: module type
 * p[3]: reg
 * p[4]: size (2|4)
 * p[5]: val
 */
plerrcode proc_lvd_aregs(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=lvd_set_aregs(dev, p[2], p[3], p[4], p[5]);

    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_aregs(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_aregs[] = "lvd_aregs";
int ver_proc_lvd_aregs = 1;
/*****************************************************************************/
/*
 * test_pulse lets all modules which are able to do so generate a
 * test pulse. All modules except sync modules and older SQDCs are able
 * to generate a test pulse. For QDCs the amplitude has to be predefined
 * by some other means (depending on type and version).
 * p[0]: argcount==1
 * p[1]: branch
 */
plerrcode proc_lvd_test_pulse(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=lvd_set_aregB(dev, 0x7e, 2, 8);

    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_test_pulse(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_test_pulse[] = "lvd_test_pulse";
int ver_proc_lvd_test_pulse = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: addr
 * p[3]: level
 *       level&0x30 ==   0: print to stdout
 *       level&0x30 ==0x10: output string to outptr
 *       level&0x30 ==0x30: do both of above
 */
plerrcode proc_lvd_modulestat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    void *xp=0;
    int res;

    if (p[3]&0x10)
        xprintf_init(&xp);

    res=lvd_modstat(dev, p[2], xp, p[3]&0xf);

    if (xp) {
        const char *p=xprintf_get(xp);
        if (p[3]&0x20)
            printf("%s", p);
        if (p[3]&0x10) {
            outptr=outstring(outptr, p);
        }
        xprintf_done(&xp);
    }

    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_modulestat(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_modulestat[] = "lvd_modstat";
int ver_proc_lvd_modulestat = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: level
 *       level&0x30 ==   0: print to stdout
 *       level&0x30 ==0x10: output string to outptr
 *       level&0x30 ==0x30: do both of above
 */
plerrcode proc_lvd_controllerstat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    void *xp=0;
    int res;

    if (p[2]&0x10)
        xprintf_init(&xp);

    res=lvd_contrstat(dev, xp, p[2]&0xf);

    if (xp) {
        const char *p=xprintf_get(xp);
        if (p[2]&0x20)
            printf("%s", p);
        if (p[2]&0x10) {
            outptr=outstring(outptr, p);
        }
        xprintf_done(&xp);
    }

    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_controllerstat(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_controllerstat[] = "lvd_contrstat";
int ver_proc_lvd_controllerstat = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: level
 *       level&0x30 ==   0: print to stdout
 *       level&0x30 ==0x10: output string to outptr
 *       level&0x30 ==0x30: do both of above
 */
plerrcode proc_lvd_cratestat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    void *xp=0;
    int res;

    if (p[2]&0x10)
        xprintf_init(&xp);

    res=lvd_cratestat(dev, xp, p[2]&0xf, (p[2]&0xf)>1?-1:0);

    if (xp) {
        const char *p=xprintf_get(xp);
        if (p[2]&0x20)
            printf("%s", p);
        if (p[2]&0x10) {
            outptr=outstring(outptr, p);
        }
        xprintf_done(&xp);
    }

    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_cratestat(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_cratestat[] = "lvd_cratestat";
int ver_proc_lvd_cratestat = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 */
plerrcode proc_lvd_localreset(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=dev->localreset(dev);

    return res?plErr_Other:plOK;
}

plerrcode test_proc_lvd_localreset(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_localreset[] = "lvd_localreset";
int ver_proc_lvd_localreset = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: addr
 * p[3]: id
 * [p[4]: bitmask for id
 * [p[5]: serial number]]
 */
plerrcode proc_lvd_force_module_id(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_force_module_id(dev, p[2], p[3],
            p[0]>3?p[4]:0xffff, p[0]>4?p[5]:-1);
}

plerrcode test_proc_lvd_force_module_id(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<3 || p[0]>5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_force_module_id[] = "lvd_force_module_id";
int ver_proc_lvd_force_module_id = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: interval in s (0s: disabled)
 */
plerrcode proc_lvd_statist_interval(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    *outptr++=lvd_statist_interval(dev, p[2]);
    return plOK;
}

plerrcode test_proc_lvd_statist_interval(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_statist_interval[] = "lvd_statist_interval";
int ver_proc_lvd_statist_interval = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: flags
 */
plerrcode proc_lvd_statist(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    outptr+=dev->statist(dev, p[2], outptr);
    return plOK;
}

plerrcode test_proc_lvd_statist(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_statist[] = "lvd_statist";
int ver_proc_lvd_statist = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * [p[2]: parseflags (see enum parsebits in lowlevel/lvd/lvdbus.h)]
 */
plerrcode proc_lvd_parseflags(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    *outptr++=dev->parseflags;
    if (p[0]>1)
        dev->parseflags=p[2];
        
    return plOK;
}

plerrcode test_proc_lvd_parseflags(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1 && p[0]>2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_parseflags[] = "lvd_parseflags";
int ver_proc_lvd_parseflags = 1;
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount>=1
 * p[1]: crate
 * p...: arbitrary header words
 */
plerrcode proc_lvd_eventheader(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int i;

    dev->num_headerwords=0; /* for safety reasons */
    dev->headerwords=realloc(dev->headerwords, (p[0]-1)*sizeof(ems_u32));
    if (!dev->headerwords && p[0]>1)
        return plErr_NoMem;

    for (i=0; i<p[0]-1; i++)
        dev->headerwords[i]=p[i+1];
    dev->num_headerwords=p[0]-1;
        
    return plOK;
}

plerrcode test_proc_lvd_eventheader(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[0]-1>63)
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_eventheader[] = "lvd_eventheader";
int ver_proc_lvd_eventheader = 1;
#endif
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount>=1
 * p[1]: crate
 */
plerrcode proc_lvd_geteventheader(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int i;

    for (i=0; i<dev->num_headerwords; i++)
        *outptr++=dev->headerwords[i];
        
    return plOK;
}

plerrcode test_proc_lvd_geteventheader(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=get_device(p[1])->num_headerwords;
    return plOK;
}

char name_proc_lvd_geteventheader[] = "lvd_geteventheader";
int ver_proc_lvd_geteventheader = 1;
#endif
/*****************************************************************************/
/*
 * p[0]: argcount (1 or 2)
 * p[1]: branch
 * [p[2]: period]
 */
plerrcode proc_lvd_controller_f1gpx_rate_trigger_period(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;
    if (p[0]>1) {
        pres=lvd_controller_f1gpx_set_rate_trigger_period(dev, p[2]);
    } else {
        pres=lvd_controller_f1gpx_get_rate_trigger_period(dev,  outptr);
        if (pres==plOK)
            outptr+=1;
    }
    return pres;
}

plerrcode test_proc_lvd_controller_f1gpx_rate_trigger_period(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1 && p[0]>2) 
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_controller_f1gpx_rate_trigger_period[] =
        "lvd_controller_f1gpx_rate_trigger_period";
int ver_proc_lvd_controller_f1gpx_rate_trigger_period = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (1 or 2)
 * p[1]: branch
 * [p[2]: period]
 */
plerrcode proc_lvd_controller_f1gpx_rate_trigger_count(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;
    if (p[0]>1) {
        pres=lvd_controller_f1gpx_set_rate_trigger_count(dev, p[2]);
    } else {
        pres=lvd_controller_f1gpx_get_rate_trigger_count(dev,  outptr);
        if (pres==plOK)
            outptr+=1;
    }
    return pres;
}

plerrcode test_proc_lvd_controller_f1gpx_rate_trigger_count(ems_u32* p)
{
    plerrcode pres;

    if  (p[0]<1 && p[0]>2) 
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_controller_f1gpx_rate_trigger_count[] =
        "lvd_controller_f1gpx_rate_trigger_count";
int ver_proc_lvd_controller_f1gpx_rate_trigger_count = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (1)
 * p[1]: branch
 * p[2]: period
 */
plerrcode proc_lvd_controller_f1gpx_extra_trig(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=lvd_controller_f1gpx_extra_trig(dev, p[2]);

    return pres;
}

plerrcode test_proc_lvd_controller_f1gpx_extra_trig(ems_u32* p)
{
    plerrcode pres;

    if  (p[0]!=2) 
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_controller_f1gpx_extra_trig[] =
        "lvd_controller_f1gpx_extra_trig";
int ver_proc_lvd_controller_f1gpx_extra_trig = 1;
/*****************************************************************************/
/*
 * p[0]: argcount (1)
 * p[1]: branch
 * p[2]: period
 */
plerrcode proc_lvd_controller_f1gpx_rate_trig(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=lvd_controller_f1gpx_rate_trig(dev, p[2]);

    return pres;
}

plerrcode test_proc_lvd_controller_f1gpx_rate_trig(ems_u32* p)
{
    plerrcode pres;

    if  (p[0]!=2) 
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_controller_f1gpx_rate_trig[] =
        "lvd_controller_f1gpx_rate_trig";
int ver_proc_lvd_controller_f1gpx_rate_trig = 1;
/*****************************************************************************/
/*****************************************************************************/
