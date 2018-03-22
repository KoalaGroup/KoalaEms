/*
 * procs/sync/plxsync/plxsync_proc.c
 * created 2007-Jul-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: plxsync_proc.c,v 1.4 2017/10/21 21:58:26 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/sync/syncdev.h"
#include "../../../lowlevel/devices.h"

#define get_device(branch) \
    (struct sync_dev*)get_gendevice(modul_sync, (branch))

RCS_REGISTER(cvsid, "procs/sync/plxsync")

/*****************************************************************************/
/*
 * p[0]: argcount==4|5
 * p[1]: branch
 * p[2]: reg
 * [p[3]: val]
 */
plerrcode proc_plxsync_plxreg(ems_u32* p)
{
    struct sync_dev* dev=get_device(p[1]);
    plerrcode pres;

    if (p[0]==2) {
        pres=dev->read_plxreg(dev, p[2], outptr);
        if (pres==plOK)
            outptr++;
    } else {
        pres=dev->write_plxreg(dev, p[2], p[3]);
    }
    return pres;
}

plerrcode test_proc_plxsync_plxreg(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_sync, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]==2?1:0;
    return plOK;
}

char name_proc_plxsync_plxreg[] = "plxsync_plxreg";
int ver_proc_plxsync_plxreg = 1;
/*****************************************************************************/
/*****************************************************************************/
