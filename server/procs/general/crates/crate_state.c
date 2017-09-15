/*
 * procs/general/crates/crate_state.c
 *
 * created 01.Jul.2009 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: crate_state.c,v 1.2 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <stdio.h>
#include <string.h>
#include <errorcodes.h>
#include <debug.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procprops.h"
#include "../../procs.h"
#include "../../../lowlevel/devices.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1...]: devicetype (String)
 *         generic (all devices)
 *         camac
 *         fastbus
 *         vme
 *         lvd
 *         perf
 *         can
 *         caenet
 *         sync
 *         pcihl
 *
 *
 * it returns a bitpattern with '1'-bits for crates which are online
 * in case of 'generic' a list of bitpatterns is returned
 */

plerrcode proc_crate_state(ems_u32* p)
{
    ems_u32 mask;
    enum Modulclass mclass, mc;
    char* devname;
    int i;

    xdrstrcdup(&devname, p+1);

    for (mclass=modul_none; mclass<=modul_invalid; mclass++) {
        if (!strcmp(devname, Modulclass_names[mclass]))
            goto found;
    }
    printf("proc_crate_state: \"%s\" is not a valid device class\n", devname);
    return plErr_ArgRange;

found:
    if (mclass!=modul_generic) {
        mask=0;
        for (i=0; i<numxdevs[mclass]; i++) {
            struct generic_dev* dev=devices[xdevices[mclass][i]];
            if (dev && dev->generic.online)
                mask|=1<<i;
        }
        *outptr++=mask;
    } else {
        for (mc=modul_none; mc<=modul_invalid; mc++) {
            mask=0;
            for (i=0; i<numxdevs[mc]; i++) {
                struct generic_dev* dev=devices[xdevices[mc][i]];
                if (dev && dev->generic.online)
                    mask|=1<<i;
            }
            *outptr++=mask;
        }
    }

    return plOK;
}

plerrcode test_proc_crate_state(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if ((xdrstrlen(p+1))!=p[0])
        return(plErr_ArgNum);
    wirbrauchen=modul_invalid+1;
    return plOK;
}

char name_proc_crate_state[]="crate_state";
int ver_proc_crate_state=1;

/*****************************************************************************/
/*****************************************************************************/
