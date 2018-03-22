/*
 * procs/lvd/lvd_read.c
 * created 10.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_read.c,v 1.5 2017/10/09 21:25:38 wuestner Exp $";

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
extern int *memberlist;

#define get_device(crate) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (crate))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: var for eventnumber
 */
plerrcode proc_lvd_read(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res, num=20000;
    ems_u32* help=outptr++;

    res=dev->readout(dev, outptr, &num, &var_list[p[2]].var.val);
    if (res>=0) {
        outptr+=num;
        *help=num;
    } else {
        outptr--;
        return plErr_Other;
    }
    return plOK;
}

plerrcode test_proc_lvd_read(ems_u32* p)
{
    int size;
    plerrcode pres;

    if (p[0]!=2) return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1])) return plErr_ArgRange;
    if ((pres=var_attrib(p[2], &size))) return pres;
    if (size!=1) return plErr_IllVarSize;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_read[] = "lvd_read";
int ver_proc_lvd_read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: crate
 * p[2]: addr
 * p[3]: reg
 * p[4]: size
 */
plerrcode proc_lvd_blockread(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
    size_t num=2000;
    ems_u32* help=outptr++;

    res=dev->read_block(dev, p[2], p[3], p[4], outptr, &num);
    if (res>=0) {
        outptr+=num;
        *help=num;
    } else {
        outptr--;
        return plErr_Other;
    }
    return plOK;
}

plerrcode test_proc_lvd_blockread(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1])) return plErr_ArgRange;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_blockread[] = "lvd_blockread";
int ver_proc_lvd_blockread = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_start(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;
printf("proc_lvd_start\n");
/*#ifdef NEVER*/
    res=dev->flush(dev);
    if (res<0) return plErr_System;
/*#endif*/

    res=dev->start(dev);
    if (res<0) return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_start(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1])) return plErr_ArgRange;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_start[] = "lvd_start";
int ver_proc_lvd_start = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_stop(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

printf("proc_lvd_stop\n");
#ifdef NEVER
    res=dev->flush(dev);
    if (res<0) return plErr_System;
#endif

    res=dev->stop(dev);
    if (res<0) return plErr_System;

    return plOK;
}

plerrcode test_proc_lvd_stop(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1])) return plErr_ArgRange;
    wirbrauchen=-1; /* unknown */
    return plOK;
}

char name_proc_lvd_stop[] = "lvd_stop";
int ver_proc_lvd_stop = 1;
/*****************************************************************************/
/*****************************************************************************/
