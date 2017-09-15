/*
 * procs/lvd/lvd_read_multi.c
 * created 10.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_read_multi.c,v 1.4 2011/04/06 20:30:33 wuestner Exp $";

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
plerrcode proc_lvd_prepare_multi(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=dev->prepare_multi(dev);
    if (res<0)
        return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_prepare_multi(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_prepare_multi[] = "lvd_prepare_multi";
int ver_proc_lvd_prepare_multi = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_cleanup_multi(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=dev->cleanup_multi(dev);
    if (res<0)
        return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_cleanup_multi(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_cleanup_multi[] = "lvd_cleanup_multi";
int ver_proc_lvd_cleanup_multi = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_stat_multi(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=dev->stat_multi(dev);
    if (res<0)
        return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_stat_multi(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_stat_multi[] = "lvd_stat_multi";
int ver_proc_lvd_stat_multi = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: var for eventnumber
 */
plerrcode proc_lvd_read_multi(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res, num=20000, rnum;
    ems_u32* help=outptr++;

    res=dev->readout_multi(dev, outptr, &num, &rnum);
    if (p[0]>1)
        var_list[p[2]].var.val=rnum;
    if (res>=0) {
        outptr+=num;
        *help=num;
    } else {
        outptr--;
        return plErr_Other;
    }
    return plOK;
}

plerrcode test_proc_lvd_read_multi(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=1) && (p[0]!=2))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[0]>1) {
        unsigned int size;
        if ((pres=var_attrib(p[2], &size)))
            return pres;
        if (size!=1)
            return plErr_IllVarSize;
    }
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_read_multi[] = "lvd_read_multi";
int ver_proc_lvd_read_multi = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_start_multi(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
printf("proc_lvd_start_multi\n");
/*#ifdef NEVER*/
    res=dev->flush(dev);
    if (res<0) return plErr_System;
/*#endif*/

    res=dev->start_multi(dev);
    if (res<0) return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_start_multi(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_start_multi[] = "lvd_start_multi";
int ver_proc_lvd_start_multi = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_stop_multi(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

printf("proc_lvd_stop_multi\n");
#ifdef NEVER
    res=dev->flush(dev);
    if (res<0) return plErr_System;
#endif

    res=dev->stop_multi(dev);
    if (res<0) return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_stop_multi(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_stop_multi[] = "lvd_stop_multi";
int ver_proc_lvd_stop_multi = 1;
/*****************************************************************************/
/*****************************************************************************/
