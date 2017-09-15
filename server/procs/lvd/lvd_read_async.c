/*
 * procs/lvd/lvd_read_async.c
 * created 2005-Mar-14 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_read_async.c,v 1.15 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../objects/var/variables.h"
#include "../../lowlevel/lvd/lvdbus.h"
/*#include "../../lowlevel/lvd/f1/f1.h"*/
#include "../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(crate) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (crate))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_read_async(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int num;
    plerrcode pres;

    if ((pres=dev->readout_async(dev, outptr, &num))==plOK)
        outptr+=num;

    return pres;
}

plerrcode test_proc_lvd_read_async(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;

    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=-1;
    return plOK;
}

char name_proc_lvd_read_async[] = "lvd_read_async";
int ver_proc_lvd_read_async = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: number of buffers
 * p[3]: size of each buffer (in bytes)
 */
plerrcode proc_lvd_prepare_async(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return dev->prepare_async(dev, p[2], p[3]);
}

plerrcode test_proc_lvd_prepare_async(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_prepare_async[] = "lvd_prepare_async";
int ver_proc_lvd_prepare_async = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_cleanup_async(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return dev->cleanup_async(dev);
}

plerrcode test_proc_lvd_cleanup_async(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_cleanup_async[] = "lvd_cleanup_async";
int ver_proc_lvd_cleanup_async = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: selftrigger
 */
plerrcode proc_lvd_start_async(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return dev->start_async(dev, p[0]>1?p[2]:0);
}

plerrcode test_proc_lvd_start_async(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_start_async[] = "lvd_start_async";
int ver_proc_lvd_start_async = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: selftrigger
 */
plerrcode proc_lvd_stop_async(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return dev->stop_async(dev, p[0]>1?p[2]:0);
}

plerrcode test_proc_lvd_stop_async(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1 && p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_stop_async[] = "lvd_stop_async";
int ver_proc_lvd_stop_async = 1;
/*****************************************************************************/
/*****************************************************************************/
