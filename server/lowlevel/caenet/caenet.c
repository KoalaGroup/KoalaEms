/*
 * ems/server/lowlevel/caenet/caenet.c
 * created: 2007-Mar-20 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: caenet.c,v 1.6 2017/10/22 20:50:39 wuestner Exp $";

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

#include "caenet.h"
#include "caennet.h"

#ifdef CAEN_A1303
#include "a1303/a1303.h"
#endif

static const char* devicename="caenet";
static const char* configname="caenetp";

RCS_REGISTER(cvsid, "lowlevel/caenet")

/*****************************************************************************/
int caenet_low_printuse(FILE* outfilepath)
{
    fprintf(outfilepath,"  [:%s=<caenpath>;<caentype>[,...]]\n", configname);
    return 1;
}
/*****************************************************************************/
struct caenet_init {
    enum caentypes caentype;
    errcode (*init)(struct caenet_dev* dev);
    const char* name;
};

struct caenet_init caen_init[]= {
#ifdef CAEN_A1303
    {caen_a1303, caen_init_a1303, "a1303"},
#endif
    {caen_none, 0, ""}
};

errcode caenet_low_init(char* arg)
{
    char *devicepath, *help;
    unsigned int i;
    int j, n;
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
            register_device(modul_can, 0, "caennet");
        } else {
            char* semi;
            struct caenet_dev* dev;
            semi=strrchr(help, ';');
            if (!semi || semi==help) {
                printf("No %s device type given for %s\n", devicename, help);
                res=Err_ArgRange;
                break;
            }
            *semi='\0'; semi++;
            dev=calloc(1, sizeof(struct caenet_dev));
            init_device_dummies(&dev->generic);
            /* find type of can device */
            for (j=0; caen_init[j].caentype!=caen_none; j++) {
                if (strcmp(semi, caen_init[j].name)==0) {
                    dev->caentype=caen_init[j].caentype;
                    dev->pathname=strdup(help);
                    res=caen_init[j].init(dev);
                    break;
                }
            }
            /* caen device not found */
            if (caen_init[j].caentype==caen_none) {
                printf("caenet device type %s not known.\n", semi);
                printf("valid types are:");
                for (j=0; caen_init[j].caentype!=caen_none; j++) {
                    printf(" '%s'", caen_init[j].name);
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
                register_device(modul_caenet, (struct generic_dev*)dev, "caennet");
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
    for (i=0; i<num_devices(modul_caenet); i++) {
        struct generic_dev* dev;
        if (find_device(modul_caenet, i, &dev)==OK) {
            dev->generic.done(dev);
            destroy_device(modul_caenet, i);
        }
    }

    return res;
}
/*****************************************************************************/
errcode caenet_low_done(void)
{
    /* devices_done will do all work */
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
