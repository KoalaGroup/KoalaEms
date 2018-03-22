/*
 * procs/lvd/lvd_read2.c
 * created 2005-Mar-14 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_read2.c,v 1.4 2017/10/09 21:25:38 wuestner Exp $";

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
extern int *memberlist;

#define get_device(crate) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (crate))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: var
 */
plerrcode proc_lvd_read2(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res, num=10000;
    int block;

    block=var_list[p[2]].var.val;

    res=dev->readout2(dev, outptr, &num, block);

    if (res>=0) {
        /*outptr+=num;*/
    } else {
        return plErr_Other;
    }
    return plOK;
}

plerrcode test_proc_lvd_read2(ems_u32* p)
{
    plerrcode pres;
    int vsize;

    if (p[0]!=2)
        return plErr_ArgNum;

    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;

    pres=var_attrib(p[2], &vsize);
    if (pres!=plOK)
        return pres;
    if (vsize!=1)
        return plErr_IllVarSize;

    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_read2[] = "lvd_read2";
int ver_proc_lvd_read2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: number of buffers
 * p[3]: size of each buffer
 */
plerrcode proc_lvd_start2(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
#if 0
    res=dev->flush(dev);
    if (res<0)
        return plErr_System;
#endif
    res=dev->start2(dev, p[2], p[3]);
    if (res<0)
        return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_start2(ems_u32* p)
{
    if (p[0]!=3) return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_start2[] = "lvd_start2";
int ver_proc_lvd_start2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_stop2(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

printf("proc_lvd_stop2\n");
#ifdef NEVER
    res=dev->flush(dev);
    if (res<0) return plErr_System;
#endif

    res=dev->stop2(dev);
    if (res<0) return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_stop2(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1])) return plErr_ArgRange;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_stop2[] = "lvd_stop2";
int ver_proc_lvd_stop2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_stat(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=dev->stat(dev);
    if (res<0) return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_stat(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1])) return plErr_ArgRange;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_stat[] = "lvd_stat";
int ver_proc_lvd_stat = 1;
/*****************************************************************************/
/*****************************************************************************/
