/*
 * procs/lvd/lvd_cratescan.c
 * created 10.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_cratescan.c,v 1.9 2011/04/06 20:30:33 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/lvd/lvdbus.h"
#include "../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(branch) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (branch))

RCS_REGISTER(cvsid, "procs/lvd")

/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: branch
 *
 * returns triples of address/modultype/id
 */
plerrcode proc_lvd_enumerate(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);

    outptr+=lvd_enumerate(dev, outptr);
    return plOK;
}

plerrcode test_proc_lvd_enumerate(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0x110*4;
    return plOK;
}

char name_proc_lvd_enumerate[] = "lvd_enumerate";
int ver_proc_lvd_enumerate = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: branch
 */
plerrcode proc_lvd_c_ident(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=lvd_c_ident(dev, outptr, outptr+1);
    if (res) {
        return plErr_BadHWAddr;
    } else {
        outptr+=2;
    }
    return plOK;
}

plerrcode test_proc_lvd_c_ident(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_c_ident[] = "lvd_c_ident";
int ver_proc_lvd_c_ident = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: branch
 * p[2]: addr ((contr<<4)|module)
 */
plerrcode proc_lvd_a_ident(ems_u32* p)
{
    struct lvd_dev* dev=get_device(p[1]);
    int res;

    res=lvd_a_ident(dev, p[2], outptr, outptr+1);
    if (res) {
        return plErr_BadHWAddr;
    } else {
        outptr+=2;
    }
    return plOK;
}

plerrcode test_proc_lvd_a_ident(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_lvd, p[1], 0))!=plOK)
        return pres;
    if (p[2]>0x1ff)
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_lvd_a_ident[] = "lvd_a_ident";
int ver_proc_lvd_a_ident = 1;
/*****************************************************************************/
/*****************************************************************************/
