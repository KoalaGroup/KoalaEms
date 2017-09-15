/*
 * lowlevel/perfspect/zelsync/zelsync_perfspect.c
 * created 29.Juni.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: zelsync_perfspect.c,v 1.7 2011/04/06 20:30:27 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rcs_ids.h>

#include "../perfspect.h"
#include "../../sync/pci_zel/sync_pci_zel.h"

RCS_REGISTER(cvsid, "lowlevel/perfspect/zelsync")

/*****************************************************************************/
#if 0
static void
perfspect_pcisync_hw_eventstart(struct perf_dev* dev)
{
    /* do nothing */
    /* should set time to zero, if necessary */
}
#endif
/*****************************************************************************/
/*
 * returns native time unit in ns
 */
static unsigned int
perfspect_pcisync_unit(struct perf_dev* dev)
{
    return 100;
}
/*****************************************************************************/
static unsigned int
perfspect_pcisync_time(struct perf_dev* dev)
{
    struct perfspect_pcisync_info *info=(struct perfspect_pcisync_info*)dev->info;
    int time=syncmod_tmc(info->sync_id);
    return time;
}
/*****************************************************************************/
#ifdef DELAYED_READ
static void
perfspect_pcisync_reset_delayed_read(struct generic_dev* dev)
{}
static int
perfspect_pcisync_read_delayed(struct generic_dev* dev)
{
    return -1;
}
static int
perfspect_pcisync_enable_delayed_read(struct generic_dev* dev, int enable)
{
    return 0;
}
#endif
/*****************************************************************************/
static errcode
perfspect_done_pcisync(struct generic_dev* gdev)
{
    struct perf_dev* dev=(struct perf_dev*)gdev;
    struct perfspect_pcisync_info *info=(struct perfspect_pcisync_info*)dev->info;
    syncmod_detach(info->sync_id);

    free(info);
    dev->info=0;
    return OK;
}
/*****************************************************************************/
errcode perfspect_init_pcisync(struct perf_dev* dev)
{
    struct perfspect_pcisync_info *info;
    errcode res;

    info=(struct perfspect_pcisync_info*)malloc(sizeof(struct perfspect_pcisync_info));
    if (!info) {
        printf("perfspect_init_pcisync: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    res=syncmod_attach(dev->pathname, &info->sync_id);
    if (res!=OK)
        return res;

    dev->generic.done=perfspect_done_pcisync;
    dev->generic.online=1;
#ifdef DELAYED_READ
    dev->generic.reset_delayed_read=perfspect_pcisync_reset_delayed_read;
    dev->generic.read_delayed=perfspect_pcisync_read_delayed;
    dev->generic.enable_delayed_read=perfspect_pcisync_enable_delayed_read;
    dev->generic.delayed_read_enabled=0;
#endif
    dev->perf_time=perfspect_pcisync_time;
    dev->perf_unit=perfspect_pcisync_unit;

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
