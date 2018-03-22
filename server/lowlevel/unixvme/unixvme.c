/*
 * lowlevel/unixvme/unixvme.c
 * created 23.Jan.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: unixvme.c,v 1.19 2017/10/22 22:47:47 wuestner Exp $";

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
#include "unixvme.h"
#include "vme.h"

#ifdef UNIXVME_DRV
#include "drv/vme_drv.h"
#endif
#ifdef UNIXVME_SIS3100
#include "sis3100/vme_sis.h"
#endif
#ifdef UNIXVME_SIS3100_UDP
#include "sis3100_udp/vme_sis_udp.h"
#endif
#ifdef UNIXVME_SIS3153
#include "sis3153usb/vme_sis_usb.h"
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/unixvme")

/*****************************************************************************/
int unixvme_low_printuse(FILE* outfilepath)
{
fprintf(outfilepath,
"  [:vmep=<vmepath;vmetype>[,...]]\n");
return 1;
}
/*****************************************************************************/

struct vme_init {
    enum vmetypes vmetype;
    errcode (*init)(struct vme_dev* dev);
    const char* name;
};

struct vme_init vme_init[]= {
#ifdef UNIXVME_DRV
    {vme_drv, vme_init_drv, "drv"},
#endif
#ifdef UNIXVME_SIS3100
    {vme_sis3100, vme_init_sis3100, "sis3100"},
#endif
#ifdef UNIXVME_SIS3100_UDP
    {vme_sis3100_udp, vme_init_sis3100_udp, "sis3100_udp"},
#endif
#ifdef UNIXVME_SIS3151_USB
    {vme_sis3151_usb, vme_init_sis3153_usb, "sis3153_usb"},
#endif
    {vme_none, 0, ""}
};

/*
 * unixvme_low_init parses the given vme device description (arg)
 * and calles the appropriate initialisation procedure.
 * 'arg' is the string gigen in the command line as "-l:vmep=<vmedev>".
 * vmedev is "<vmetype>;<vmepath>"
 * vmetype is interpreted here, vmepath is used in the called procedure.
 * More than one vme device can be given, they are separated by ','
 */
errcode unixvme_low_init(char* arg)
{
    char *vmepath, *help;
    unsigned int i, j, n;
    errcode res;

    T(unixvme_init)

    if((!arg) || (cgetstr(arg, "vmep", &vmepath) < 0)){
        printf("no VME device given\n");
        /*return(Err_ArgNum);*/
        return(OK);
    }
    D(D_USER, printf("VME devices given as >%s<\n", vmepath); )

    help=vmepath;
    res=0; n=0;
    while (help) {
        char *comma;
        comma=strchr(help, ',');
        if (comma) *comma='\0';
        if (!strcmp(help, "none")) {
            printf("No VME for vmedev %d\n", n);
            register_device(modul_vme, 0, "vme");
        } else {
            char* semi;
            struct vme_dev* dev;
            semi=strrchr(help, ';');
            if (!semi || semi==help) {
                printf("No VME device type given for %s\n", help);
                res=Err_ArgRange;
                break;
            }
            *semi='\0'; semi++;
            dev=calloc(1, sizeof(struct vme_dev));
            /* find type of vme device */
            for (j=0; vme_init[j].vmetype!=vme_none; j++) {
                if (strcmp(semi, vme_init[j].name)==0) {
                    dev->vmetype=vme_init[j].vmetype;
                    dev->pathname=strdup(help);
                    res=vme_init[j].init(dev);
                    break;
                }
            }
            /* vme device not found */
            if (vme_init[j].vmetype==vme_none) {
                printf("vme device type %s not known.\n", semi);
                printf("valid types are:");
                for (j=0; vme_init[j].vmetype!=vme_none; j++) {
                    printf(" '%s'", vme_init[j].name);
                }
                printf("\n");
                res=Err_ArgRange;
            }
            if (res==OK) {
                int nn, i;
                nn=(void**)&dev->DUMMY-(void**)&dev->generic.done;
                for (i=0; i<nn; i++) {
                    if (((void**)&dev->generic.done)[i]==0) {
                        printf("ERROR: VME procedure idx=%d for %s "
                                "not set\n", i, help);
                        free(dev);
                        return Err_Program;
                    }
                }
                register_device(modul_vme, (struct generic_dev*)dev, "vme");
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
    free(vmepath);
    if (res) goto errout;

    return(OK);

errout:
    for (i=0; i<num_devices(modul_vme); i++) {
        struct generic_dev* dev;
        if (find_device(modul_vme, i, &dev)==OK) {
            dev->generic.done(dev);
            destroy_device(modul_vme, i);
        }
    }

    return res;
}
/*****************************************************************************/
errcode unixvme_low_done()
{
    /* devices_done will do all work */
    return(OK);
}
/*****************************************************************************/
/*****************************************************************************/
