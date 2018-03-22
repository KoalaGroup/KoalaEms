/*
 * procs/lvd/lvd_read_single.c
 * created 10.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_read_single.c,v 1.17 2017/10/20 22:55:08 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/lvd/lvdbus.h"
/*#include "../../lowlevel/lvd/f1/f1.h"*/
#include "../../lowlevel/devices.h"
#include "../../objects/var/variables.h"

#define get_device(crate) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (crate))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_prepare_single(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return dev->prepare_single(dev);
}

plerrcode test_proc_lvd_prepare_single(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_prepare_single[] = "lvd_prepare_single";
int ver_proc_lvd_prepare_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_cleanup_single(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return dev->cleanup_single(dev);
}

plerrcode test_proc_lvd_cleanup_single(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_cleanup_single[] = "lvd_cleanup_single";
int ver_proc_lvd_cleanup_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2|3
 * p[1]: crate
 * p[2]: num (maximum number of words to read)
 * [p[3]: flags]
 * flags:
 *  0x80000000: perform handshake (if no trigger procedure is used)
 *  0x40000000: readout error is ignored
 *  0x20000000: lost triggers are fatal
 *  0x10000000: event number not received is fatal
 */
plerrcode proc_lvd_read_single(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int num=p[2];
    ems_u32* help=outptr++;
    plerrcode pres;

    if ((pres=dev->readout_single(dev, outptr, &num, p[0]>2?p[3]:0))==plOK) {
        outptr+=num;
        *help=num;
    }

    return pres;
}

plerrcode test_proc_lvd_read_single(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[2]>0x1fffffff) /* 2^28 words */
        return plErr_ArgRange;
    wirbrauchen=p[2]+1;
    return plOK;
}

char name_proc_lvd_read_single[] = "lvd_read_single";
int ver_proc_lvd_read_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1|2
 * p[1]: crate
 * p[2]: selftrigger
 */
plerrcode proc_lvd_start_single(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=dev->start_single(dev, p[0]>1?p[2]:0);
    if (pres!=plOK)
        printf("lvd_start_single: pres=%d\n", pres);
    return pres;
}

plerrcode test_proc_lvd_start_single(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK) {
        printf("test_lvd_start_single: pres=%d\n", pres);
        return pres;
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_start_single[] = "lvd_start_single";
int ver_proc_lvd_start_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: selftrigger
 */
plerrcode proc_lvd_stop_single(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    /*printf("proc_lvd_stop_single\n");*/
#ifdef NEVER
    if (dev->flush(dev)<0)
        return plErr_System;
#endif

    return dev->stop_single(dev, p[0]>1?p[2]:0);
}

plerrcode test_proc_lvd_stop_single(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_stop_single[] = "lvd_stop_single";
int ver_proc_lvd_stop_single = 1;
/*****************************************************************************/
/*****************************************************************************/
