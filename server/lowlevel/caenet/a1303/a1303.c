/*
 * lowlevel/caenet/a1303/a1303.c
 * created: 2007-Mar-20 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: a1303.c,v 1.7 2011/04/06 20:30:22 wuestner Exp $";

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
#include "../../../main/scheduler.h"
#include <../../../commu/commu.h>
#include <rcs_ids.h>
#include "../caennet.h"
#include "a1303.h"
#include <dev/pci/a1303.h>

struct caen_a1303_info {
    int p;
    /*struct seltaskdescr* st;*/
};

static ems_u8* buffer_=0;

RCS_REGISTER(cvsid, "lowlevel/caenet/a1303")

/*****************************************************************************/
static plerrcode
a1303_buffer(struct caenet_dev* dev, ems_u8** buf)
{
    if (!buffer_)
        buffer_=malloc(CAENET_BLOCKSIZE);
    *buf=buffer_;
    return plOK;
}
/*****************************************************************************/
static plerrcode
a1303_read(struct caenet_dev* dev, ems_u8* buf, int* len)
{
    struct caen_a1303_info* info=(struct caen_a1303_info*)dev->info;
    ssize_t res;

    do {
        res=read(info->p, buf, *len);
    } while (res==-1 && errno==EINTR);
    if (res<0) {
        printf("caenet_a1303_read(%s): %s\n", dev->pathname, strerror(errno));
        return plErr_System;
    }
    *len=res;
    return plOK;
}
/*****************************************************************************/
static plerrcode
a1303_write(struct caenet_dev* dev, ems_u8* buf, int len)
{
    struct caen_a1303_info* info=(struct caen_a1303_info*)dev->info;
    int res;

    res=write(info->p, buf, len);
    if (res!=len) {
        if (res<0) {
            printf("caenet_a1303_write(%s): %s\n", dev->pathname,
                strerror(errno));
        } else {
            printf("caenet_a1303_write(%s): only %d of %d ytes written\n",
                dev->pathname, res, len);
        }
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
a1303_write_read(struct caenet_dev* dev, void *wbuf, int wsize,
        void *rbuf, int *rsize)
{
    plerrcode res;
    if ((res=a1303_write(dev, wbuf, wsize))!=plOK)
        return res;
    if ((res=a1303_read(dev, rbuf, rsize))!=plOK)
        return res;
    return plOK;
}
/*****************************************************************************/
static plerrcode
a1303_echo(struct caenet_dev* dev, int *num)
{
    plerrcode res;
    ems_u8* buffer;
    int len, i;

    a1303_buffer(dev, &buffer);

    len=CAENET_BLOCKSIZE;
    if ((res=a1303_read(dev, buffer, &len))!=plOK)
        return res;
    for (i=2; i<len; i++)
        buffer[i]++;
    if ((res=a1303_write(dev, buffer, len))!=plOK)
        return res;
    *num=len;
    return plOK;
}
/*****************************************************************************/
static plerrcode
a1303_reset(struct caenet_dev* dev)
{
    struct caen_a1303_info* info=(struct caen_a1303_info*)dev->info;

    if (ioctl(info->p, A1303_IOCTL_RESET)<0) {
        printf("caenet_a1303_reset(%s): %s\n",
                dev->pathname, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
a1303_timeout(struct caenet_dev* dev, int timeout_)
{
    struct caen_a1303_info* info=(struct caen_a1303_info*)dev->info;
    unsigned long timeout=timeout_;

    if (ioctl(info->p, A1303_IOCTL_TIMEOUT, timeout)<0) {
        printf("caenet_a1303_timeout(%s): %s\n",
                dev->pathname, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
a1303_led(struct caenet_dev* dev)
{
    struct caen_a1303_info* info=(struct caen_a1303_info*)dev->info;

    if (ioctl(info->p, A1303_IOCTL_LED)<0) {
        printf("caenet_a1303_led(%s): %s\n",
                dev->pathname, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static int
init_a1303(struct caenet_dev* dev)
{
    struct caen_a1303_info* info=(struct caen_a1303_info*)dev->info;

    info->p=open(dev->pathname, O_RDWR/*|O_NONBLOCK*/);
    if (info->p<0) {
        printf("caen_init_a1303: open \"%s\": %s\n", dev->pathname,
                strerror(errno));
        return -1;
    }
    if (fcntl(info->p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(caen_init_a1303 \"%s\", FD_CLOEXEC): %s\n", dev->pathname,
                strerror(errno));
    }

    if (a1303_timeout(dev, 100)!=plOK)
        goto error;

    return 0;

error:
    close(info->p);
    info->p=-1;
    return -1;
}
/*****************************************************************************/
static errcode
caen_done_a1303(struct generic_dev* gdev)
{
    struct caenet_dev* dev=(struct caenet_dev*)gdev;
    struct caen_a1303_info* info=(struct caen_a1303_info*)dev->info;

#if 0
    if (info->st) {
        sched_delete_select_task(info->st);
    }
#endif
    close(info->p);

    if (dev->pathname)
        free(dev->pathname);
    dev->pathname=0;
    free(info);
    dev->info=0;

    return OK;
}
/*****************************************************************************/
errcode
caen_init_a1303(struct caenet_dev* dev)
{
    struct caen_a1303_info* info;
    errcode res;

    info=(struct caen_a1303_info*)calloc(1, sizeof(struct caen_a1303_info));
    if (!info) {
        printf("caen_init_a1303: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    if (init_a1303(dev)<0) {
        res=Err_System;
        goto error;
    }

    dev->generic.done   =caen_done_a1303;
    dev->read           =a1303_read;
    dev->write          =a1303_write;
    dev->write_read     =a1303_write_read;
    dev->echo           =a1303_echo;
    dev->reset          =a1303_reset;
    dev->timeout        =a1303_timeout;
    dev->led            =a1303_led;
    dev->buffer         =a1303_buffer;

    dev->generic.online=1;

    return OK;

error:
    close(info->p);
    free(info);
    dev->info=0;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
