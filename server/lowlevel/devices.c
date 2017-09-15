/*
 * lowlevel/devices.c
 * created 01.Oct.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: devices.c,v 1.22 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rcs_ids.h>
#include "devices.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct generic_dev** devices;
int numdevs;

int* xdevices[modul_invalid]; /* index in devices[] */
int numxdevs[modul_invalid]; /* number of devices */

RCS_REGISTER(cvsid, "lowlevel")

/*****************************************************************************/
void devices_init(void)
{
    int i;
    devices=0;
    numdevs=0;
    for (i=0; i<modul_invalid; i++) {
        xdevices[i]=0;
        numxdevs[i]=0;
    }
}
/*****************************************************************************/
errcode devices_done(void)
{
    errcode res=OK;
    int i;
    for (i=0; i<numdevs; i++) {
        errcode lres;
        if (devices[i]) {
            lres=devices[i]->generic.done(devices[i]);
            free(devices[i]);
            if (res==OK) res=lres;
        }
    }
    free(devices);

    for (i=0; i<modul_invalid; i++) {
        if (xdevices[i]) free(xdevices[i]);
    }
    return res;
}
/*****************************************************************************/
int register_device(Modulclass class, struct generic_dev* dev,
        const char* caller)
{
    struct generic_dev** help;
    int* ihelp;
    int i;

    printf("register_device(class=%d (%s) called from %s)\n",
            class, Modulclass_names[class], caller);
    if (class>=modul_invalid) {
        printf("register_device(class=%d): invalid class\n", class);
        return -1;
    }
    if (dev) {
        dev->generic.class=class;
        dev->generic.alldev_idx=numdevs;
        dev->generic.dev_idx=numxdevs[class];
        printf("register device class %d: %d/%d\n", class, numdevs, numxdevs[class]);
    }

    help=malloc((numdevs+1)*sizeof(struct generic_dev*));
    for (i=0; i<numdevs; i++)
        help[i]=devices[i];
    if (devices)
        free(devices);
    devices=help;
    devices[numdevs]=dev;

    ihelp=malloc((numxdevs[class]+1)*sizeof(int));
    for (i=0; i<numxdevs[class]; i++)
        ihelp[i]=xdevices[class][i];
    if (xdevices[class])
        free(xdevices[class]);
    xdevices[class]=ihelp;
    xdevices[class][numxdevs[class]]=numdevs;

    numxdevs[class]++;
    numdevs++;

    return 0;
}
/*****************************************************************************/
void destroy_device(Modulclass class, int crate)
{
    free(devices[xdevices[class][crate]]);
    devices[xdevices[class][crate]]=0;
}
/*****************************************************************************/
static int
_find_device(Modulclass class, unsigned int crate, struct generic_dev **dev)
{
#if 0
    printf("find_device(class=%d crate=%d), dev=%p\n", class, crate, dev);
#endif

    *dev=0;
    if (class>=modul_invalid) {
        printf("find_device(class=%d crate=%d): invalid class\n",
                class, crate);
        return 2;
    }
    if (crate>=numxdevs[class]) {
        printf("find_device(class=%d crate=%d): no such crate\n",
                class, crate);
        return 1;
    }
    if (!devices[xdevices[class][crate]]) {
        printf("find_device(class=%d crate=%d): not valid\n",
                class, crate);
        return 1;
    }

    *dev=devices[xdevices[class][crate]];
    return 0;
}
errcode
find_device(Modulclass class, unsigned int crate, struct generic_dev **dev)
{
    struct generic_dev *device;
    errcode err;
    int res;

    res=_find_device(class, crate, &device);
    switch (res) {
    case 0: err=OK; break;
    case 1: err=Err_ArgRange; break;
    case 2:
    default: err=Err_Program;
    }
    if (dev)
        *dev=device;
    return err;
}
plerrcode
find_odevice(Modulclass class, unsigned int crate, struct generic_dev **dev)
{
    struct generic_dev *device;
    int res;

    res=_find_device(class, crate, &device);
    if (res) {
        plerrcode pres;
        switch (res) {
        case 1: pres=plErr_ArgRange; break;
        case 2:
        default: pres=plErr_Program;
        }
        return pres;
    }
    if (!device->generic.online) {
        printf("find_device(class=%d crate=%d): not online\n",
                class, crate);
        return plErr_Offline;
    }
    if (dev)
        *dev=device;
    return plOK;
}
/*****************************************************************************/
#ifdef DELAYED_READ
static int
dummy_enable_delayed_read(struct generic_dev* dev, int val)
{
    return 0;
}

static void
dummy_reset_delayed_read(struct generic_dev* gdev)
{}

static int
dummy_read_delayed(struct generic_dev* gdev)
{
    return -1;
}

void reset_delayed_read(void)
{
    int i;
    for (i=0; i<numdevs; i++) {
        if (devices[i] && devices[i]->generic.delayed_read_enabled)
                devices[i]->generic.reset_delayed_read(devices[i]);
    }
}

int read_delayed(void)
{
    int err=0, i;

    for (i=0; i<numdevs; i++) {
        if (devices[i] && devices[i]->generic.delayed_read_enabled)
                err=err||devices[i]->generic.read_delayed(devices[i]);
    }
    return err;
}
#endif

static plerrcode
dummy_init_jtag_dev(struct generic_dev* dev, void* jdev)
{
    return plErr_BadModTyp;
}

void
init_device_dummies(struct dev_generic* dev)
{
#ifdef DELAYED_READ
    dev->delayed_read_enabled=0;
    dev->reset_delayed_read=dummy_reset_delayed_read;
    dev->read_delayed=dummy_read_delayed;
    dev->enable_delayed_read=dummy_enable_delayed_read;
#endif
    dev->init_jtag_dev=dummy_init_jtag_dev;
}

/*****************************************************************************/
enum Modulclass
find_devclass(const char* name)
{
    int i;

    for (i=0; Modulclass_names[i]; i++) {
        if (strcasecmp(name, Modulclass_names[i])==0) {
            printf("find_devclass: \"%s\" --> %d \n", name, i);
            return i;
        }
    }
    printf("find_devclass: class \"%s\" not found\n", name);
    return -1;
}
/*****************************************************************************/
/*****************************************************************************/
