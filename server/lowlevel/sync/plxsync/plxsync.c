/*
 * lowlevel/sync/plxsync/plxsync.c
 * created 2007-Jul-02 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: plxsync.c,v 1.9 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <rcs_ids.h>

#include <errorcodes.h>
#include "../../../main/nowstr.h"
#include "../../../commu/commu.h"
#include "../../jtag/jtag_lowlevel.h"
#include "../sync_init.h"
#include "plxsync.h"

#include "dev/pci/plxsync_var.h"
#include "dev/pci/plxsync_map.h"
#include "dev/pci/plx9054reg.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct opaque_data_plxsync {
    int p;
    unsigned long long int sleepmagic;
};

volatile ems_u32 dummy;
#define ofs(what, elem) ((off_t)&(((what *)0)->elem))
#define offset(elem) ofs(struct plxsync_reg, elem)

RCS_REGISTER(cvsid, "lowlevel/sync/plxsync")

/*****************************************************************************/
static int
plxsync_reset(struct sync_dev* dev)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    int res;

    res=ioctl(info->p, PLXSYNC_RESET, 0);
    if (res) {
        printf("plxsync_reset: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static plerrcode
plxsync_get_eventcount(struct sync_dev* dev, u_int32_t* data)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ctrl_reg req;
    int res;

    req.offset=offset(evc);
    res=ioctl(info->p, PLXSYNC_CTRL_READ, &req);
    if (res) {
        printf("plxsync_get_eventcount: %s\n", strerror(errno));
        return plErr_System;
    }
    *data=req.val;
    return plOK;
}
/*****************************************************************************/
static plerrcode
plxsync_set_eventcount(struct sync_dev* dev, ems_u32 data)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ctrl_reg req;
    int res;

    req.offset=offset(evc);
    req.val=data;
    res=ioctl(info->p, PLXSYNC_CTRL_WRITE, &req);
    if (res) {
        printf("plxsync_set_eventcount: %s\n", strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
plxsync_write_reg(struct sync_dev* dev, ems_u32 addr, int size, ems_u32 data)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ctrl_reg req;
    int res;

    if (size!=4)
        return plErr_ArgRange;
    req.offset=addr;
    req.val=data;
    res=ioctl(info->p, PLXSYNC_CTRL_WRITE, &req);
    if (res) {
        printf("plxsync_ctrlwrite(0x%08x, 0x%08x): %s\n", addr, data,
                strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
plxsync_read_reg(struct sync_dev* dev, u_int32_t addr, int size,
        u_int32_t* data)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ctrl_reg req;
    int res;

    if (size!=4)
        return plErr_ArgRange;
    req.offset=addr;
    res=ioctl(info->p, PLXSYNC_CTRL_READ, &req);
    if (res) {
        printf("plxsync_ctrlread(0x%08x): %s\n", addr, strerror(errno));
        return plErr_System;
    }
    *data=req.val;
    return plOK;
}
/*****************************************************************************/
static plerrcode
plxsync_write_plxreg(struct sync_dev* dev, ems_u32 addr, ems_u32 data)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ctrl_reg req;
    int res;

    req.offset=addr;
    req.val=data;
    printf("plxsync_reg[%s] %02x <== %08x\n", dev->pathname, addr, req.val);
    res=ioctl(info->p, PLXSYNC_PLX_WRITE, &req);
    if (res) {
        printf("plxsync_write_plx(0x%08x, 0x%08x): %s\n", addr, data,
                strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
plxsync_read_plxreg(struct sync_dev* dev, u_int32_t addr, u_int32_t* data)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ctrl_reg req;
    int res;

    req.offset=addr;
    res=ioctl(info->p, PLXSYNC_PLX_READ, &req);
    if (res) {
        printf("plxsync_read_plx(0x%08x): %s\n", addr, strerror(errno));
        return plErr_System;
    }
    printf("plxsync_reg[%s] %02x: %08x\n", dev->pathname, addr, req.val);
    *data=req.val;
    return plOK;
}
/*****************************************************************************/
static int
plxsync_ident(struct sync_dev* dev)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ident ident;
    u_int32_t driverversion;

    if (ioctl(info->p, PLXSYNC_DRIVERVERSION, &driverversion)<0) {
        printf("plxsync_driverversion(%s): %s\n",
                dev->pathname, strerror(errno));
        return -1;
    }
    if (ioctl(info->p, PLXSYNC_IDENT, &ident)<0) {
        printf("plxsync_ident(%s): %s\n",
                dev->pathname, strerror(errno));
        return -1;
    }

    printf("plxsync driverversion: %d.%d\n",
        (driverversion>>16)&0xffff,
        driverversion&0xffff);
    printf("plxsync ident: 0x%08x\n", ident.ident);
    printf("plxsync serial: %d|%d\n", ident.serial&0xffff,
            (ident.serial>>16)&0xffff);

    return 0;
}
/****************************************************************************/
static int
jt_sleep(struct jtag_dev* jdev)
{
    struct sync_dev* dev=(struct sync_dev*)jdev->dev;
    ems_u32 val=0;

    plxsync_read_reg(dev, 0, 4, &val);
    return val;
}
/****************************************************************************/
static int
jt_action_plxsync(struct jtag_dev* jdev, int tms, int tdi, int* tdo)
{
    struct opaque_data_plxsync* opaque=
            (struct opaque_data_plxsync*)jdev->opaque;
    ems_u32 csr, tdo_;

    csr=PLXSYNC_JT_C|(tms?PLXSYNC_JT_TMS:0)|(tdi?PLXSYNC_JT_TDI:0);

    if (ioctl(opaque->p, PLXSYNC_JTAG_PUT, &csr))
        return -1;
    if (ioctl(opaque->p, PLXSYNC_JTAG_GET, &csr))
        return -1;
    tdo_=!!(csr&PLXSYNC_JT_TDO);

    if (tdo!=0)
        *tdo=tdo_;
    return 0;
}
/****************************************************************************/
static int
jt_data_plxsync(struct jtag_dev* jdev, ems_u32* v)
{
    struct opaque_data_plxsync* opaque=
            (struct opaque_data_plxsync*)jdev->opaque;
    int res;
    res=ioctl(opaque->p, PLXSYNC_JTAG_DATA, v);
    return res;
}
/****************************************************************************/
static void
jt_free(struct jtag_dev* jdev)
{
    free(jdev->opaque);
    jdev->opaque=0;
}
/*****************************************************************************/
static plerrcode
plxsync_init_jtag_dev(struct generic_dev* gdev, void* jdev_)
{
    struct sync_dev* dev=(struct sync_dev*)gdev;
    struct plxsync_info* info=(struct plxsync_info*)dev->info;
    struct jtag_dev* jdev=(struct jtag_dev*)jdev_;
    void* opaque=0;

    jdev->opaque=0;
    jdev->jt_data=0;
    jdev->jt_action=0;
    jdev->jt_free=jt_free;
    jdev->jt_sleep=0;
    jdev->jt_mark=0;

    if (jdev->addr!=0) {
        printf("plxsync_init_jtag: illegal address %d\n", jdev->addr);
        return plErr_ArgRange;
    }
    jdev->jt_data=jt_data_plxsync;
    jdev->jt_action=jt_action_plxsync;
    jdev->jt_free=jt_free;
    jdev->jt_sleep=jt_sleep;
    opaque=malloc(sizeof(struct opaque_data_plxsync));
    if (opaque==0) {
        printf("plxsync_init_jtag: %s\n", strerror(errno));
        return plErr_System;
    }
    ((struct opaque_data_plxsync*)opaque)->p=info->p;
    jdev->opaque=opaque;
    return plOK;
}
/*****************************************************************************/
static plerrcode
plxsync_modstat(struct sync_dev* dev, int level)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;
    struct plxsync_ctrl_reg req;
    const char *s;
    ems_u32 ident=0;

    printf("\n=== PLXSYNC_STAT %s for device %s ===\n", nowstr(), dev->pathname);
    s="ident";
    req.offset=offset(ident);
    if (ioctl(info->p, PLXSYNC_CTRL_READ, &req)<0) {
        printf("plxsync: read %s: %s\n", s, strerror(errno));
    } else {
        ident=req.val;
    }
    if (level>1)
        printf("  %6s: 0x%08x\n", "ident", ident);

    if (level>=0) {
        s="sr";
        req.offset=offset(sr);
        if (ioctl(info->p, PLXSYNC_CTRL_READ, &req)<0)
            printf("plxsync: read %s: %s\n", s, strerror(errno));
        else
            printf("  %6s: 0x%08x\n", s, req.val);
    }
    if (level>0) {
        s="cr";
        req.offset=offset(cr);
        if (ioctl(info->p, PLXSYNC_CTRL_READ, &req)<0)
            printf("plxsync: read %s: %s\n", s, strerror(errno));
        else
            printf("  %6s: 0x%08x\n", s, req.val);
    }
    if (level>=0) {
        s="intsr";
        req.offset=offset(intsr);
        if (ioctl(info->p, PLXSYNC_CTRL_READ, &req)<0)
            printf("plxsync: read %s: %s\n", s, strerror(errno));
        else
            printf("  %6s: 0x%08x\n", s, req.val);
    }
    if (level>1) {
        s="serial";
        req.offset=offset(serial);
        if (ioctl(info->p, PLXSYNC_CTRL_READ, &req)<0)
            printf("plxsync: read %s: %s\n", s, strerror(errno));
        else
            printf("  %6s: 0x%08x\n", s, req.val);
    }
    if (level>=0) {
        s="evc";
        req.offset=offset(evc);
        if (ioctl(info->p, PLXSYNC_CTRL_READ, &req)<0)
            printf("plxsync: read %s: %s\n", s, strerror(errno));
        else
            printf("  %6s: 0x%08x\n", s, req.val);
    }
    if (ident>=0x10000000) {
        s="tpat";
        req.offset=offset(tpat);
        if (ioctl(info->p, PLXSYNC_CTRL_READ, &req)<0)
            printf("plxsync: read %s: %s\n", s, strerror(errno));
        else
            printf("  %6s: 0x%08x\n", s, req.val);
    }

    if (level>1) {
        if (ioctl(info->p, PLXSYNC_DUMP)<0)
            printf("plxsync: dump: %s\n", strerror(errno));
    }
    return plOK;
}
/*****************************************************************************/
static int
plxsync_init(struct sync_dev* dev)
{
    struct plxsync_info *info=(struct plxsync_info*)dev->info;

    info->p=open(dev->pathname, O_RDWR, 0);
    if (info->p<0) {
        printf("plxsync_init: open(%s) %s\n", dev->pathname, strerror(errno));
        return -1;
    }
    if (fcntl(info->p, F_SETFD, FD_CLOEXEC)<0) {
        printf("plxsync_init: fcntl(%s, FD_CLOEXEC): %s\n", dev->pathname,
                strerror(errno));
    }
    plxsync_ident(dev);
    plxsync_reset(dev);
    return 0;
}
/*****************************************************************************/
static errcode
sync_done_plx(struct generic_dev* gdev)
{
    struct sync_dev* dev=(struct sync_dev*)gdev;
    struct plxsync_info* info=(struct plxsync_info*)dev->info;

    close(info->p);
    if (dev->pathname)
        free(dev->pathname);
    free(info);
    dev->info=0;

    return OK;
}
/*****************************************************************************/
errcode
sync_init_plx(struct sync_dev* dev)
{
    struct plxsync_info* info;
    errcode res;

    info=(struct plxsync_info*) calloc(1, sizeof(struct plxsync_info));
    if (!info) {
        printf("init_plxsync: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    if (plxsync_init(dev)<0) {
        res=Err_System;
        goto error;
    }

    dev->generic.online=1;
    dev->generic.done         =sync_done_plx;
    dev->generic.init_jtag_dev=plxsync_init_jtag_dev;
    dev->reset                =plxsync_reset;
    dev->get_eventcount       =plxsync_get_eventcount;
    dev->set_eventcount       =plxsync_set_eventcount;
    dev->read_reg             =plxsync_read_reg;
    dev->write_reg            =plxsync_write_reg;
    dev->module_state         =plxsync_modstat;
    dev->read_plxreg          =plxsync_read_plxreg;
    dev->write_plxreg         =plxsync_write_plxreg;

    return OK;

error:
    close(info->p);
    free(info);
    dev->info=0;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/

