/*
 * procs/test/delayedread/delayread.c
 *
 * created 06.Oct.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: delayread.c,v 1.5 2017/10/21 22:03:43 wuestner Exp $";

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

RCS_REGISTER(cvsid, "procs/test/delayedread")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1...]: devicetype (String)
 *         generic (all devices)
 *         camac
 *         fastbus
 *         vme
 * p[x+1]: bitmask for crates (ignored if devicetype=='generic')
 * p[x+2]: 1: enable 0: disable
 */
plerrcode proc_DelayRead(ems_u32* p)
{
    ems_u32* pp, mask;
    unsigned int i;
    int enable;
    char* devname;
    Modulclass mclass;

    pp=xdrstrcdup(&devname, p+1);
    mask=pp[0];
    enable=pp[1];
    if (!strcmp(devname, "generic")) mclass=modul_generic;
    else if (!strcmp(devname, "camac")) mclass=modul_camac;
    else if (!strcmp(devname, "fastbus")) mclass=modul_fastbus;
    else if (!strcmp(devname, "vme")) mclass=modul_vme;
    else {
        printf("proc_DelayRead: \"%s\" is not a valid device class\n", devname);
        return plErr_ArgRange;
    }
    if (mclass==modul_generic) {
        for (i=0; i<numdevs; i++) {
            if (devices[i]) {
                *outptr++=i;
                *outptr++=devices[i]->generic.enable_delayed_read(devices[i],
                        enable);
            }
        }
    } else {
        for (i=0; i<numxdevs[mclass]; i++) {
            if ((1<<i)&mask) {
                struct generic_dev* dev=devices[xdevices[mclass][i]];
                if (dev) {
                    *outptr++=i;
                    *outptr++=dev->generic.enable_delayed_read(dev, enable);
                }
            }
        }
    }
    return plOK;
}

plerrcode test_proc_DelayRead(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if ((xdrstrlen(p+1)+2)!=p[0])
        return(plErr_ArgNum);
    wirbrauchen=-1;
    return plOK;
}

#ifdef PROCPROPS
static procprop DelayRead_prop={0, 1, "void", 0};

procprop* prop_proc_DelayRead(void)
{
    return(&DelayRead_prop);
}
#endif
char name_proc_DelayRead[]="DelayRead";
int ver_proc_DelayRead=1;

/*****************************************************************************/
/*****************************************************************************/
