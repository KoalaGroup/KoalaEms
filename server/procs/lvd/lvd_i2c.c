/*
 * procs/lvd/lvd_i2c.c
 * created 2009-Oct-21 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_i2c.c,v 1.3 2017/10/09 21:25:38 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../lowlevel/lvd/lvdbus.h"
#include "../../lowlevel/devices.h"

extern ems_u32* outptr;

#define get_device(branch) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (branch))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: port
 * p[3]: region 0: ID memory 1: diagnostics
 * p[4]: addr
 * p[5]: size
 */
plerrcode proc_lvd_i2c_read(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=lvd_i2c_read(dev, p[2], p[3], p[4], p[5], outptr);
    if (pres==plOK)
        outptr+=p[5];
    return pres;
}

plerrcode test_proc_lvd_i2c_read(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[2]>1 || p[3]>2)
        return plErr_ArgRange;
    if (p[4]>255 || p[4]+p[5]>256)
        return plErr_ArgRange;

    wirbrauchen=p[5];
    return plOK;
}

char name_proc_lvd_i2c_read[] = "lvd_i2c_read";
int ver_proc_lvd_i2c_read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: branch
 * p[2]: port
 * p[3]: region 0: ID memory 1: diagnostics
 * p[4]: addr
 * p[5...]: data (one byte per word)
 */
plerrcode proc_lvd_i2c_write(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    return lvd_i2c_write(dev, p[2], p[3], p[4], p[0]-4, p+5);
}

plerrcode test_proc_lvd_i2c_write(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[2]>1 || p[3]>2)
        return plErr_ArgRange;
    if (p[4]>255 || p[4]+p[0]-4>256)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_i2c_write[] = "lvd_i2c_write";
int ver_proc_lvd_i2c_write = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p]2]: port
 */
plerrcode proc_lvd_i2c_dump(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    return lvd_i2c_dump(dev, p[2]);
}

plerrcode test_proc_lvd_i2c_dump(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[2]>1)
        return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_i2c_dump[] = "lvd_i2c_dump";
int ver_proc_lvd_i2c_dump = 1;
/*****************************************************************************/
