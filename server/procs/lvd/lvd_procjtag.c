/*
 * procs/lvd/lvd_procjtag.c
 * created 2005-Aug-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_procjtag.c,v 1.3 2017/10/09 21:25:38 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/lvd/lvdbus.h"
#include "../../lowlevel/jtag/jtag_tools.h"
#include "../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int *memberlist;

#define get_device(branch) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (branch))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: trace
 */
plerrcode proc_lvd_jtag_trace(ems_u32* p)
{
    struct generic_dev* dev=get_gendevice(modul_lvd, p[1]);
    plerrcode pres;

    pres=jtag_trace(dev, p[2], outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}

plerrcode test_proc_lvd_jtag_trace(ems_u32* p)
{
    if (p[0]!=2)
        return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_jtag_trace[] = "lvd_jtag_trace";
int ver_proc_lvd_jtag_trace = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: ci 0: daq module, 1: lvd crate controller 2: PCI interface
 * p[3]: addr
 */
plerrcode proc_lvd_jtag_list(ems_u32* p)
{
    /*struct lvd_dev* dev=get_device(p[1]);*/
    struct generic_dev* dev=get_gendevice(modul_lvd, p[1]);
    plerrcode pres;

    pres=jtag_list(dev, p[2], p[3]);
    return pres;
}

plerrcode test_proc_lvd_jtag_list(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_jtag_list[] = "lvd_jtag_list";
int ver_proc_lvd_jtag_list = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: ci 0: daq module, 1: lvd crate controller 2: PCI interface
 * p[3]: addr
 */
plerrcode proc_lvd_jtag_check(ems_u32* p)
{
    /*struct lvd_dev* dev=get_device(p[1]);*/
    struct generic_dev* dev=get_gendevice(modul_lvd, p[1]);
    plerrcode pres;

    pres=jtag_check(dev, p[2], p[3]);
    return pres;
}

plerrcode test_proc_lvd_jtag_check(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_jtag_check[] = "lvd_jtag_check";
int ver_proc_lvd_jtag_check = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: ci 0: daq module, 1: lvd crate controller 2: PCI interface
 * p[3]: addr
 * p[4]: chip
 */
plerrcode proc_lvd_jtag_id(ems_u32* p)
{
    /*struct lvd_dev* dev=get_device(p[1]);*/
    struct generic_dev* dev=get_gendevice(modul_lvd, p[1]);
    plerrcode pres;

    pres=jtag_read_id(dev, p[2], p[3], p[4], outptr);
    if (pres==plOK)
        outptr++;
    return pres;
}

plerrcode test_proc_lvd_jtag_id(ems_u32* p)
{
    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_jtag_id[] = "lvd_jtag_id";
int ver_proc_lvd_jtag_id = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: ci 0: daq module, 1: lvd crate controller 2: PCI interface
 * p[3]: addr
 * p[4]: chip_idx
 * p[5...]: filename
 */
plerrcode proc_lvd_jtag_read(ems_u32* p)
{
    /*struct lvd_dev* dev=get_device(p[1]);*/
    struct generic_dev* dev=get_gendevice(modul_lvd, p[1]);
    char* name;
    plerrcode pres;

    xdrstrcdup(&name, p+5);
    pres=jtag_read(dev, p[2], p[3], p[4], name);
    free(name);

    return pres;
}

plerrcode test_proc_lvd_jtag_read(ems_u32* p)
{
    if (p[0]<5)
        return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;
    if (xdrstrlen(p+5)!=p[0]-4)
        return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_jtag_read[] = "lvd_jtag_read";
int ver_proc_lvd_jtag_read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: ci 0: daq module, 1: lvd crate controller 2: PCI interface
 * p[3]: addr
 * p[4]: chip_idx
 * p[5...]: filename
 */
plerrcode proc_lvd_jtag_write(ems_u32* p)
{
    struct generic_dev* dev=get_gendevice(modul_lvd, p[1]);
    char* name;
    plerrcode pres;

    xdrstrcdup(&name, p+5);
    pres=jtag_write(dev, p[2], p[3], p[4], name);
    free(name);

    return pres;
}

plerrcode test_proc_lvd_jtag_write(ems_u32* p)
{
    if (p[0]<5)
        return plErr_ArgNum;
    if (!valid_device(modul_lvd, p[1]))
        return plErr_ArgRange;
    if (xdrstrlen(p+5)!=p[0]-4)
        return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_jtag_write[] = "lvd_jtag_write";
int ver_proc_lvd_jtag_write = 1;
/*****************************************************************************/
/*****************************************************************************/
