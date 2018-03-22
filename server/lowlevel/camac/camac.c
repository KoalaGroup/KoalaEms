/*
 * lowlevel/camac/camac.c
 * created 04.Oct.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: camac.c,v 1.20 2017/10/21 23:06:52 wuestner Exp $";

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
#include "camac.h"
#include "camac_init.h"

RCS_REGISTER(cvsid, "lowlevel/camac")

/*****************************************************************************/
int camac_low_printuse(FILE* outfilepath)
{
fprintf(outfilepath,
"  [:camacp=<camacpath>;<camactype>[;<rawmempart>][,...]]\n"
"                                    (rawmempart=n/m, n<=m)\n");
return 1;
}
/*****************************************************************************/

struct camac_init {
    enum camactypes camactype;
    errcode (*init)(struct camac_dev* dev, char* rawmempart);
    const char* name;
};

struct camac_init camac_init[]= {
#ifdef CAMAC_CC16
    {camac_cc16, camac_init_cc16, "cc16"},
#endif
#ifdef CAMAC_PCICAMAC
    {camac_pcicamac, camac_init_pcicamac, "pcicamac"},
#endif
#ifdef CAMAC_JORWAY73A
    {camac_jorway73a, camac_init_jorway73a, "jorway73a"},
#endif
#ifdef CAMAC_LNXDSP
    {camac_lnxdsp, camac_init_lnxdsp, "lnxdsp"},
#endif
#ifdef CAMAC_VCC2117
    {camac_vcc2117, camac_init_vcc2117, "vcc2117"},
#endif
#ifdef CAMAC_SIS5100
    {camac_sis5100, camac_init_sis5100, "sis5100"},
#endif
    {camac_none, 0, ""}
};

static struct camac_raw_procs*
get_raw_procs_dummy(__attribute__((unused)) struct camac_dev* dev)
{
    return 0;
}

errcode camac_low_init(char* arg)
{
    char *devicepath, *help;
    unsigned int i;
    int j, n, camacdelay=0;
    errcode res;

    T(camac_init)

    if((!arg) || (cgetstr(arg, "camacp", &devicepath) < 0)){
        printf("no CAMAC device given\n");
        /*return(Err_ArgNum);*/
        return(OK);
    }
    D(D_USER, printf("camac devices given as >%s<\n", devicepath); )

    {
        long cd;
        if (cgetnum(arg, "camd", &cd))
            cd=0;
        camacdelay=cd;
        printf("CAMAC delay=%d\n", camacdelay);
    }

    help=devicepath;
    res=0; n=0;
    while (help) {
        char *comma;

        comma=strchr(help, ',');
        if (comma) *comma='\0';
        if (!strcmp(help, "none")) {
            printf("No CAMAC for camacdev %d\n", n);
            register_device(modul_camac, 0, "camac");
        } else {
            char *semi, *semi2;
            char *camacname, *rawpart;
            struct camac_dev* dev;
            semi=strrchr(help, ';');
            if (!semi || semi==help) {
                if (semi==help) {
                    printf("SEMI==HELP\n");
                }
                printf("No camac device type given for %s\n", help);
                res=Err_ArgRange;
                break;
            }
            *semi++='\0';

            semi2=strrchr(help, ';');
            if (semi2) {
                *semi2++='\0';
                camacname=semi2;
                rawpart=semi;
            } else {
                camacname=semi;
                rawpart=0;
            }

            dev=calloc(1, sizeof(struct camac_dev));
            init_device_dummies(&dev->generic);
            dev->get_raw_procs=get_raw_procs_dummy;
            dev->camdelay=camacdelay;
            /* find type of camac device */
            for (j=0; camac_init[j].camactype!=camac_none; j++) {
                if (strcmp(camacname, camac_init[j].name)==0) {
                    dev->camactype=camac_init[j].camactype;
                    dev->pathname=strdup(help);
                    res=camac_init[j].init(dev, rawpart);
                    break;
                }
            }
            /* camac device not found */
            if (camac_init[j].camactype==camac_none) {
                printf("camac device type %s not known.\n", semi);
                printf("valid types are:");
                for (j=0; camac_init[j].camactype!=camac_none; j++) {
                    printf(" '%s'", camac_init[j].name);
                }
                printf("\n");
                res=Err_ArgRange;
            }
            if (res==OK) {
                int nn, i;
                nn=(void**)&dev->DUMMY-(void**)&dev->generic.done;
                for (i=0; i<nn; i++) {
                    if (((void**)&dev->generic.done)[i]==0) {
                        printf("ERROR: CAMAC procedure idx=%d for %s "
                                "not set\n", i, help);
                        return Err_Program;
                    }
                }
                register_device(modul_camac, (struct generic_dev*)dev, "camac");
            } else {
                if (dev->pathname)
                    free(dev->pathname);
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
    for (i=0; i<num_devices(modul_camac); i++) {
        struct generic_dev* dev;
        if (find_device(modul_camac, i, &dev)==OK) {
            dev->generic.done(dev);
            destroy_device(modul_camac, i);
        }
    }

    return res;
}
/*****************************************************************************/
errcode camac_low_done()
{
    /* devices_done will do all work */
    return(OK);
}
/*****************************************************************************/
/*****************************************************************************/
