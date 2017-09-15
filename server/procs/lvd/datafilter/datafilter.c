/*
 * procs/lvd/datafilter/datafilter.c
 * created 2010-Feb-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: datafilter.c,v 1.2 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/datafilter/datafilter.h"
#include "../../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_device(branch) \
    ((struct lvd_dev*)get_gendevice(modul_lvd, (branch)))

RCS_REGISTER(cvsid, "procs/lvd/datafilter")

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_lvd_listdatafilter(ems_u32* p)
{
    outptr+=lvd_list_datafilter(outptr);
    return plOK;
}

plerrcode test_proc_lvd_listdatafilter(ems_u32* p)
{
    if (p[0])
        return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_lvd_listdatafilter[] = "lvd_listdatafilter";
int ver_proc_lvd_listdatafilter = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1]: crate
 * p[2]: filter (string)
 * p[...]: arbitrary data used for filter procedure
 */
/*
 * filter is a predefined procedure which can filter the event data from
 * one LVDS crate. It can alter the data and change the amount of data.
 * It has to regenerate the LVDS data structure (header words)
 */
plerrcode proc_lvd_datafilter(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    char *name;
    ems_u32 *pp=xdrstrcdup(&name, p+2);
    plerrcode pres;

    pres=lvd_add_datafilter(dev, name, pp, p[0]-1-xdrstrlen(p+2), 0);
        
    return pres;
}

plerrcode test_proc_lvd_datafilter(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<2)
        return plErr_ArgNum;
    if (p[0]<1+xdrstrlen(p+2))
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_datafilter[] = "lvd_datafilter";
int ver_proc_lvd_datafilter = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 */
plerrcode proc_lvd_getdatafilter(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    outptr+=lvd_get_datafilter(dev, outptr);

    return plOK;
}

plerrcode test_proc_lvd_getdatafilter(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_getdatafilter[] = "lvd_getdatafilter";
int ver_proc_lvd_getdatafilter = 1;
/*****************************************************************************/
/*
 * p[0]: argcount>=1
 * p[1]: crate
 */
plerrcode proc_lvd_cleardatafilter(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    lvd_clear_datafilter(dev);

    return plOK;
}

plerrcode test_proc_lvd_cleardatafilter(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_device(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_lvd_cleardatafilter[] = "lvd_cleardatafilter";
int ver_proc_lvd_cleardatafilter = 1;
/*****************************************************************************/
/*****************************************************************************/
