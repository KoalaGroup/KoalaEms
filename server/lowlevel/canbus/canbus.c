/*
 * ems/server/lowlevel/canbus/canbus.c
 * created 9.1.2006 pk
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: canbus.c,v 1.7 2017/10/21 23:23:06 wuestner Exp $";

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

#include "canbus.h"
#include "canbus_init.h"
#include "canbus_msg.h"

static const char* devicename="CANBUS";
static const char* configname="canbusp";

RCS_REGISTER(cvsid, "lowlevel/canbus")

/*****************************************************************************/
int canbus_low_printuse(FILE* outfilepath)
{
    fprintf(outfilepath,"  [:%s=<canpath>;<cantype>[,...]]\n", configname);
    return 1;
}
/*****************************************************************************/
struct canbus_init {
    enum cantypes cantype;
    errcode (*init)(struct canbus_dev* dev);
    const char* name;
};

struct canbus_init can_init[]= {
#ifdef CAN_PEAKPCI
    {can_peakpci, can_init_peakpci, "peakpci"},
#endif
#ifdef CAN_CAN331
    {can_can331, can_init_esd331, "can331"},
#endif
    {can_none, 0, ""}
};

errcode canbus_low_init(char* arg)
{
    char *devicepath, *help;
    unsigned int i, j, n;
    errcode res;

    T(can_low_init)

    if ((!arg) || (cgetstr(arg, configname, &devicepath) < 0)){
        printf("no %s device given\n", devicename);
        return OK;
    }
    D(D_USER, printf("%s devices given as >%s<\n", devicename, devicepath); )

    res=0; n=0;
    help=devicepath;
    while (help) {
        char *comma;
        comma=strchr(help, ',');
        if (comma) *comma='\0';
        if (!strcmp(help, "none")) {
            printf("No %s device for device %d\n", devicename, n);
            register_device(modul_can, 0, "canbus");
        } else {
            char* semi;
            struct canbus_dev* dev;
            semi=strrchr(help, ';');
            if (!semi || semi==help) {
                printf("No %s device type given for %s\n", devicename, help);
                res=Err_ArgRange;
                break;
            }
            *semi='\0'; semi++;
            dev=calloc(1, sizeof(struct canbus_dev));
            init_device_dummies(&dev->generic);
            /* find type of can device */
            for (j=0; can_init[j].cantype!=can_none; j++) {
                if (strcmp(semi, can_init[j].name)==0) {
                    dev->cantype=can_init[j].cantype;
                    dev->pathname=strdup(help);
                    res=can_init[j].init(dev);
                    break;
                }
            }
            /* can device not found */
            if (can_init[j].cantype==can_none) {
                printf("can device type %s not known.\n", semi);
                printf("valid types are:");
                for (j=0; can_init[j].cantype!=can_none; j++) {
                    printf(" '%s'", can_init[j].name);
                }
                printf("\n");
                res=Err_ArgRange;
            }
            if (res==OK) {
                int nn, i;
                nn=(void**)&dev->DUMMY-(void**)&dev->generic.done;
                for (i=0; i<nn; i++) {
                    if (((void**)&dev->generic.done)[i]==0) {
                        printf("ERROR: %s procedure idx=%d for %s "
                                        "not set\n", devicename, i, help);
                        free(dev);
                        return Err_Program;
                    }
                }
                register_device(modul_can, (struct generic_dev*)dev, "canbus");
            } else {
                if (dev->pathname)
                    free(dev->pathname);
                free(dev);
                break;
            }
        }
        n++;
        help=comma?comma+1:0;
    }
    free(devicepath);

    if (res)
        goto errout;

    return OK;

errout:
    for (i=0; i<num_devices(modul_can); i++) {
        struct generic_dev* dev;
        if (find_device(modul_can, i, &dev)==OK) {
            dev->generic.done(dev);
            destroy_device(modul_can, i);
        }
    }

    return res;
}
/*****************************************************************************/
errcode canbus_low_done(void)
{
    /* devices_done will do all work */
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
