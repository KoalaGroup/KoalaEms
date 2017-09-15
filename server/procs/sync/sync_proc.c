/*
 * procs/sync/sync_proc.c
 * created 2007-Jul-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sync_proc.c,v 1.3 2011/04/06 20:30:34 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/sync/syncdev.h"
#include "../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(branch) \
    (struct sync_dev*)get_gendevice(modul_sync, (branch))

RCS_REGISTER(cvsid, "procs/sync")

/*****************************************************************************/
/*
 * p[0]: argcount==1/2
 * p[1]: branch
 * [p[2]: new eventcount]
 */
plerrcode proc_sync_eventcount(ems_u32* p)
{
    struct sync_dev* dev=get_device(p[1]);
    plerrcode pres;

    pres=dev->get_eventcount(dev, outptr);
    if (pres!=plOK)
        return pres;
    outptr++;
    if (p[0]>1)
        pres=dev->set_eventcount(dev, p[2]);
    return pres;
}

plerrcode test_proc_sync_eventcount(ems_u32* p)
{
    plerrcode pres;
    if ((p[0]<1) || (p[0]>2))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_sync, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_sync_eventcount[] = "sync_eventcount";
int ver_proc_sync_eventcount = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3|4
 * p[1]: branch
 * p[2]: reg
 * p[3]: size (2|4)
 * [p[4]: val]
 */
plerrcode proc_sync_reg(ems_u32* p)
{
    struct sync_dev* dev=get_device(p[1]);
    plerrcode pres;

    if (p[0]==3) {
        pres=dev->read_reg(dev, p[2], p[3], outptr);
        if (pres==plOK)
            outptr++;
    } else {
        pres=dev->write_reg(dev, p[2], p[3], p[4]);
    }
    return pres;
}

plerrcode test_proc_sync_reg(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=3) && (p[0]!=4))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_sync, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]==3?1:0;
    return plOK;
}

char name_proc_sync_reg[] = "sync_reg";
int ver_proc_sync_reg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: branch
 * p[2]: level
 */
plerrcode proc_sync_modulestate(ems_u32* p)
{
    struct sync_dev* dev=get_device(p[1]);
    int res;

    res=dev->module_state(dev, p[2]);

    return res?plErr_Other:plOK;
}

plerrcode test_proc_sync_modulestate(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_sync, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_sync_modulestate[] = "sync_modstat";
int ver_proc_sync_modulestate = 1;
/*****************************************************************************/
/*****************************************************************************/
