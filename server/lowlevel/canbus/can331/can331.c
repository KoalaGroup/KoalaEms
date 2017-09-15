/*
 * lowlevel/canbus/can331/esd331.c
 * created 2007-Jul-02 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: can331.c,v 1.3 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../main/scheduler.h"
#include <../../../commu/commu.h>
#include "../canbus_init.h"

struct esd331_info {
    int p;
    struct seltaskdescr* st;
};

RCS_REGISTER(cvsid, "lowlevel/canbus/can331")

/*****************************************************************************/
static int
esd331_init(struct canbus_dev* dev) {
    struct esd331_info* info=(struct esd331_info*)dev->info;

    info->p=open(dev->pathname, O_RDWR/*|O_NONBLOCK*/);
    if (info->p<0) {
        printf("esd331_init: open \"%s\": %s\n", dev->pathname,
                strerror(errno));
        return -1;
    }
    if (fcntl(info->p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(esd331_init \"%s\", FD_CLOEXEC): %s\n", dev->pathname,
                strerror(errno));
        goto error;
    }





    return 0;
error:
    close(info->p);
    info->p=-1;
    return -1;
    
}
/*****************************************************************************/
static errcode
can_done_esd331(struct generic_dev* gdev)
{
    struct canbus_dev* dev=(struct canbus_dev*)gdev;
    struct esd331_info* info=(struct esd331_info*)dev->info;

    if (info->st) {
        sched_delete_select_task(info->st);
    }
    close(info->p);
    if (dev->pathname)
        free(dev->pathname);
    free(info);
    dev->info=0;

    return OK;
}
/*****************************************************************************/
errcode
can_init_esd331(struct canbus_dev* dev)
{
    struct esd331_info* info;
#if 0
    union callbackdata cbdata;
#endif
    errcode res;

    info=(struct esd331_info*) calloc(1, sizeof(struct esd331_info));
    if (!info) {
        printf("can_init_esd331: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    if (esd331_init(dev)<0) {
        res=Err_System;
        goto error;
    }

#if 0
    cbdata.p=dev;
    info->st=sched_insert_select_task(esd331_message, cbdata,
            "esd331", info->p, select_read
    #ifdef SELECT_STATIST
            , 1
    #endif
            );
    if (!info->st) {
        printf("can_init_esd331: cannot install callback for message\n");
        res=Err_System;
        goto error;
    }
#else
    info->st=0;
#endif

    dev->generic.done   =can_done_esd331;
#if 0
    dev->read           =esd331_read;
    dev->write          =esd331_write;
    dev->write_read     =esd331_write_read;
    dev->write_read_bulk=esd331_write_read_bulk;
    dev->unsol          =esd331_unsol;
    dev->debug          =esd331_debug;
#endif

    dev->send_unsol=0;
    dev->do_debug=0;
    dev->generic.online=1;

    return OK;

error:
    if (info->st) {
        sched_delete_select_task(info->st);
    }
    close(info->p);
    free(info);
    dev->info=0;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
