/*
 * lowlevel/lvd/lvd.c
 * created 22.Aug.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd.c,v 1.10 2011/04/06 20:30:25 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include "lvd.h"
#include "lvdbus.h"

#ifdef LVD_SIS1100
#   include "sis1100/sis1100_lvd.h"
#endif

#ifdef DMALLOC
#   include <dmalloc.h>
#endif

static const char* devicename="LVD";
static const char* configname="lvdp";

RCS_REGISTER(cvsid, "lowlevel/lvd")

/*****************************************************************************/
int lvd_low_printuse(FILE* outfilepath)
{
    fprintf(outfilepath, "  [:%s=<lvdpath;lvdtype>[,...]]\n", configname);
    return 1;
}
/*****************************************************************************/
struct lvd_init {
    enum lvdtypes lvdtype;
    errcode (*init)(struct lvd_dev* dev);
    const char* name;
    int second_link;
};

struct lvd_init lvd_init[]= {
#ifdef LVD_SIS1100
    {lvd_sis1100, lvd_init_sis1100, "sis1100", 0},
    {lvd_sis1100, lvd_init_sis1100X, "sis1100X", 1},
#endif
#ifdef LVD_sonstwas
    {lvd_sonstwas, lvd_init_sonstwas, "sonstwas", 0},
#endif
    {lvd_none, 0, "", 0}
};
/*****************************************************************************/
static int
find_type(const char *s)
{
    int i;
    for (i=0; lvd_init[i].lvdtype!=lvd_none; i++) {
        if (strcmp(s, lvd_init[i].name)==0) {
            return i;
        }
    }
    return -1;
}
/*****************************************************************************/
static errcode
check_lvd_procs(struct lvd_dev *dev)
{
    int n, i;
    n=(void**)&dev->DUMMY-(void**)&dev->generic.done;
    for (i=0; i<n; i++) {
        if (((void**)&dev->generic.done)[i]==0) {
            printf("ERROR: %s procedure idx=%d for %s "
                            "not set\n", devicename, i, dev->pathname);
            return Err_Program;
        }
    }
    return OK;
}
/*****************************************************************************/
static errcode
create_lvd_device(const char *s)
{
    struct lvd_dev *dev;
    char* semi;
    int idx;
    errcode res;

    semi=strrchr(s, ';');
    if (!semi || semi==s) {
        printf("No %s device type given for %s\n", devicename, s);
        return Err_ArgRange;
    }

    *semi='\0'; semi++;
    idx=find_type(semi);
    if (idx<0) {
        printf("lvd device type %s not known.\n", semi);
        printf("valid types are:");
        for (idx=0; lvd_init[idx].lvdtype!=lvd_none; idx++) {
            printf(" '%s'", lvd_init[idx].name);
        }
        printf("\n");
        return Err_ArgRange;
    }

    dev=calloc(1, sizeof(struct lvd_dev));
    dev->lvdtype=lvd_init[idx].lvdtype;
    dev->pathname=strdup(s);
    init_device_dummies(&dev->generic);

    res=lvd_init[idx].init(dev);
    if (res!=OK)
        goto error;

    if (lvd_init[idx].second_link) {
        if (dev->pathname)
            free(dev->pathname);
        free(dev);
    } else {
        res=check_lvd_procs(dev);
        if (res==OK)
            register_device(modul_lvd, (struct generic_dev*)dev, "lvd");
    }

error:
    if (res!=OK) {
        if (dev->pathname)
            free(dev->pathname);
        free(dev);
    }
    return res;
}
/*****************************************************************************/
errcode lvd_low_init(char* arg)
{
    char *devicepath, *help;
    errcode res;
    int n;

    T(lvd_low_init)

    sis1100_init();

    if ((!arg) || (cgetstr(arg, configname, &devicepath) < 0)){
        printf("no %s device given\n", devicename);
        return(OK);
    }
    D(D_USER, printf("%s devices given as >%s<\n", devicename, devicepath); )

    res=OK; n=0;
    help=devicepath;
    while (help && res==OK) {
        char *comma;
        comma=strchr(help, ',');
        if (comma) *comma='\0';

        if (!strcmp(help, "none")) {
            printf("No %s device for device %d\n", devicename, n);
            register_device(modul_lvd, 0, "lvd");
        } else {
            res=create_lvd_device(help);
        }

        n++;
        help=comma?comma+1:0;
    }

    free(devicepath);

    if (res!=OK) {
        int i;
        for (i=0; i<num_devices(modul_lvd); i++) {
            struct generic_dev* dev;
            if (find_device(modul_lvd, i, &dev)==OK) {
                dev->generic.done(dev);
                destroy_device(modul_lvd, i);
            }
        }
    }
    return res;
}
/*****************************************************************************/
errcode lvd_low_done()
{
    printf("== lowlevel/lvd/lvd.c:lvd_low_done ==\n");
    /* devices_done will do all work */

    sis1100_done();

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
