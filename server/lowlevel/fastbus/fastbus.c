/*
 * lowlevel/fastbus/fastbus.c
 * created 23.May.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fastbus.c,v 1.11 2011/04/06 20:30:23 wuestner Exp $";

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
#include "../devices.h"
#include "fastbus.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifdef FASTBUS_CHI
#include "chi/chifastbus.h"
#endif

#ifdef FASTBUS_SFI
#include "sfi/sfifastbus.h"
#endif

#ifdef FASTBUS_FVSBI
#include "fvsbi/fvsbifastbus.h"
#endif

#ifdef FASTBUS_SIS3100_SFI
#include "sis3100_sfi/sis3100_sfifastbus.h"
#endif

RCS_REGISTER(cvsid, "lowlevel/fastbus")

/*****************************************************************************/
int fastbus_low_printuse(FILE* outfilepath)
{
fprintf(outfilepath,
"  [:fastbusp=<fbvmepath;fbtype>[,...]]\n");
return 1;
}
/*****************************************************************************/

struct fb_init {
    enum fastbustypes fbtype;
    errcode (*init)(struct fastbus_dev* dev);
    const char* name;
};

struct fb_init fb_init[]= {
#ifdef FASTBUS_CHI
    {fastbus_chi, fastbus_init_chi, "chi"},
#endif
#ifdef FASTBUS_SFI
    {fastbus_sfi, fastbus_init_sfi, "sfi"},
#endif
#ifdef FASTBUS_FVSBI
    {fastbus_fvsbi, fastbus_init_fvsbi, "fvsbi"},
#endif
#ifdef FASTBUS_SIS3100_SFI
    {fastbus_sis3100_sfi, fastbus_init_sis3100_sfi, "sis3100_sfi"},
#endif
    {fastbus_none, 0, ""}
};

errcode fastbus_low_init(char* arg)
{
    char *devicepath, *help;
    int i, j, n;
    errcode res;

    T(fastbus_init)

    if((!arg) || (cgetstr(arg, "fastbusp", &devicepath) < 0)){
        printf("no FASTBUS device given\n");
        /*return(Err_ArgNum);*/
        return(OK);
    }
    D(D_USER, printf("FASTBUS devices given as >%s<\n", devicepath); )

    help=devicepath;
    res=0; n=0;
    while (help) {
        char *comma;
        comma=strchr(help, ',');
        if (comma) *comma='\0';
        if (!strcmp(help, "none")) {
            printf("No FASTBUS for fastbusdev %d\n", n);
            register_device(modul_fastbus, 0, "fb");
        } else {
            char* semi;
            struct fastbus_dev* dev;
            semi=strrchr(help, ';');
            if (!semi || semi==help) {
                printf("No FASTBUS device type given for %s\n", help);
                res=Err_ArgRange;
                break;
            }
            *semi='\0'; semi++;
            dev=calloc(1, sizeof(struct fastbus_dev));
            init_device_dummies(&dev->generic);
            /* find type of fastbus device */
            for (j=0; fb_init[j].fbtype!=fastbus_none; j++) {
                if (strcmp(semi, fb_init[j].name)==0) {
                    dev->fastbustype=fb_init[j].fbtype;
                    dev->pathname=strdup(help);
                    res=fb_init[j].init(dev);
                    break;
                }
            }
            /* fastbus device not found */
            if (fb_init[j].fbtype==fastbus_none) {
                printf("fastbus device type %s not known.\n", semi);
                printf("valid types are:");
                for (j=0; fb_init[j].fbtype!=fastbus_none; j++) {
                    printf(" '%s'", fb_init[j].name);
                }
                printf("\n");
                res=Err_ArgRange;
            }
            if (res==OK) {
                int nn, i;
                nn=(void**)&dev->DUMMY-(void**)&dev->generic.done;
                for (i=0; i<nn; i++) {
                    if (((void**)&dev->generic.done)[i]==0) {
                        printf("ERROR: FASTBUS procedure idx=%d for %s "
                                "not set\n", i, help);
                        return Err_Program;
                    }
                }
                register_device(modul_fastbus, (struct generic_dev*)dev, "fb");
            } else {
                free(dev);
                break;
            }
        }
        n++;
        if (comma)
            help=comma+1;
        else
            help=0;
    }
    free(devicepath);
    if (res) goto errout;

    return(OK);

    errout:
    for (i=0; i<num_devices(modul_fastbus); i++) {
        struct generic_dev* dev;
        if (find_device(modul_fastbus, i, &dev)==OK) {
            dev->generic.done(dev);
            destroy_device(modul_fastbus, i);
        }
    }

    return res;
}
/*****************************************************************************/
errcode fastbus_low_done()
{
    /* devices_done will do all work */
    return(OK);
}
/*****************************************************************************/
/*****************************************************************************/
