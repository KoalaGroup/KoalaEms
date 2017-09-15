/*
 * lowlevel/sync/sync.c
 * created 2007-Jul-02 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sync.c,v 1.4 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/*#include <fcntl.h>
#include <sys/ioctl.h>*/
#include <getcap.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "sync.h"
#include "sync_init.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static const char* devicename="Sync";
static const char* configname="syncp";

RCS_REGISTER(cvsid, "lowlevel/sync")

/*****************************************************************************/
int sync_low_printuse(FILE* outfilepath)
{
    fprintf(outfilepath, "  [:%s=<syncpath>;<synctype>[,...]]\n", configname);
    return 1;
}
/*****************************************************************************/
struct sync_init {
    enum synctypes synctype;
    errcode (*init)(struct sync_dev* dev);
    const char* name;
};

struct sync_init synch_init[]= {
#ifdef SYNC_PLXSYNC
    {sync_plx, sync_init_plx, "plx"},
#endif
#ifdef SYNC_sonstwas
    {sync_sonstwas, sync_init_sonstwas, "sonstwas"},
#endif
    {sync_none, 0, ""}
};

errcode sync_low_init(char* arg)
{
    char *devicepath, *help;
    int i, j, n;
    errcode res;

    T(sync_low_init)

    if ((!arg) || (cgetstr(arg, configname, &devicepath) < 0)){
        printf("no %s device given\n", devicename);
        return(OK);
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
            register_device(modul_sync, 0, "synch");
        } else {
            char* semi;
            struct sync_dev* dev;
            semi=strrchr(help, ';');
            if (!semi || semi==help) {
                printf("No %s device type given for %s\n", devicename, help);
                res=Err_ArgRange;
                break;
            }
            *semi='\0'; semi++;
            dev=calloc(1, sizeof(struct sync_dev));
            init_device_dummies(&dev->generic);
            /* find type of sync device */
            for (j=0; synch_init[j].synctype!=sync_none; j++) {
                if (strcmp(semi, synch_init[j].name)==0) {
                    dev->synctype=synch_init[j].synctype;
                    dev->pathname=strdup(help);
                    res=synch_init[j].init(dev);
                    break;
                }
            }
            /* sync device not found */
            if (synch_init[j].synctype==sync_none) {
                printf("sync device type %s not known.\n", semi);
                printf("valid types are:");
                for (j=0; synch_init[j].synctype!=sync_none; j++) {
                    printf(" '%s'", synch_init[j].name);
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
                register_device(modul_sync, (struct generic_dev*)dev, "synch");
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
    for (i=0; i<num_devices(modul_sync); i++) {
        struct generic_dev* dev;
        if (find_device(modul_sync, i, &dev)==OK) {
            dev->generic.done(dev);
            destroy_device(modul_sync, i);
        }
    }

    return res;
}
/*****************************************************************************/
errcode sync_low_done()
{
    /* devices_done will do all work */
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
