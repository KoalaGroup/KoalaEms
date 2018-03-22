/*
 * trigger/pci/lvd/lvdtrigger_async.c
 * created 25.Jun.2005 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvdtrigger_async.c,v 1.21 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "../../../lowlevel/lvd/lvd_access.h"
#include "../../../lowlevel/lvd/sis1100/sis1100_lvd.h"

struct lvd_data {
    ems_u32 module_idx;
    ml_entry* module;
    struct lvd_dev* dev;
    struct lvd_sis1100_info* dev_info;
};

struct private {
    int numbranches;
    struct lvd_data* data;
    int flags;
};

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "trigger/pci/lvd")

/*****************************************************************************/
static void
trig_pcilvd_async_callback(struct lvd_dev *dev,
    const struct lvdirq_callbackdata *lvd_data,
    void* data)
{
    struct triggerinfo* trinfo=(struct triggerinfo*)data;
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    int i;

#if 0
    printf("trig_pcilvd_async_callback vector=%d\n", vector);
#endif
    if (!(lvd_data->mask&ZELLVD_DDMA_IRQ))
        printf("trig_pcilvd_async_callback mask=0x%x\n", lvd_data->mask);

    if (lvd_data->seq==-1) {
        printf("trig_pcilvd_async_callback: seq==-1\n");
        //dev->acknowledge_lvdirq(dev, ZELLVD_DDMA_IRQ);
        return;
    }

    trinfo->trigger=1<<31;
    for (i=0; i<priv->numbranches; i++) {
        if (priv->data[i].dev==dev) {
            trinfo->trigger=1<<i;
            break;
        }
    }
    trinfo->time=lvd_data->time;
    trinfo->time_valid=1;

    /* sanity check */
    if (info->ra.activeblock!=-1) {
        complain("trig_pcilvd_async_callback: block %d active!\n"
            "new block=%d old block=%d\n", info->ra.activeblock,
            lvd_data->seq, info->ra.dma_newblock);
        fatal_readout_error();
    }
    info->ra.dma_newblock=lvd_data->seq;

    trinfo->cb_proc(trinfo->cb_data);
}
/*****************************************************************************/
static int
unregister_irq(struct lvd_dev* dev)
{
    return dev->unregister_lvdirq(dev, ZELLVD_DDMA_IRQ,
                trig_pcilvd_async_callback);
}
/*****************************************************************************/
static void
suspend_trig_pcilvd_async(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 0
    printf("suspend_trig_pcilvd_async\n");
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;

        unregister_irq(dev);
    }
}
/*****************************************************************************/
static void
remove_trig_pcilvd_async(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("remove_trig_pcilvd_async\n");
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;

        unregister_irq(dev);
    }
}
/*****************************************************************************/
static int
register_irq(struct lvd_dev* dev, struct triggerinfo* trinfo, int noclear)
{
    return dev->register_lvdirq(dev, ZELLVD_DDMA_IRQ, ZELLVD_DDMA_IRQ,
                trig_pcilvd_async_callback, trinfo,
                "trig_pcilvd_async_callback", noclear);
}
/*****************************************************************************/
static void
reactivate_trig_pcilvd_async(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 0
    printf("reactivate_trig_pcilvd_async\n");
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;

        register_irq(dev, trinfo, 1);
    }
}
/*****************************************************************************/
static errcode
insert_trig_pcilvd_async(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

    printf("insert_trig_pcilvd_async\n");
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;
        ems_u32 cr;

        printf("trig_pcilvd_async[%d]: request demand DMA IRQ\n", i);
        /* bit 30 is used as flag for ddma */
        
        if (register_irq(dev, trinfo, 0)<0)
            return Err_System;

        /* check whether controller is already enabled */
        if (lvd_cc_r(dev, cr, &cr)<0)
            return Err_System;
        if (cr==0) {
            plerrcode pres;
            printf("insert_trig_pcilvd: starting controller\n");
            pres=lvd_start_controller(dev);
            if (pres!=plOK) {
                printf("insert_trig_pcilvd: lvd_start_controller returns %d\n",
                        pres);
                return Err_System;
            }
        } else {
            printf("insert_trig_pcilvd: controller already started\n");
        }

    }
    return OK;
}
/*****************************************************************************/
static plerrcode
done_trig_pcilvd_async(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    free(priv->data);
    free(tinfo->private);
    return plOK;
}
/*****************************************************************************/
#if 0
/*
 * get_trig_pcilvd_async will never be called
 * (all work is done in trig_pcilvd_async_callback)
 */
static int
get_trig_pcilvd_async(struct triggerinfo* trinfo)
{
    printf("!!! get_trig_pcilvd_async called (but it should not)!!!\n");
    return -1;
}
#endif
/*****************************************************************************/
static void
reset_trig_pcilvd_async(__attribute__((unused)) struct triggerinfo* trinfo)
{
/*
    nothing to be done because irq is auto-acknowledged
*/
#if 0
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct lvd_data* dat;
    struct lvd_dev* dev;
    int branch, trigg=trinfo->trigger;

    branch=0;
    while (!(trigg&1)) {
        branch++;
        trigg>>=1;
    }
    dat=priv->data+branch;
    dev=dat->dev;

    dev->acknowledge_lvdirq(dev, ZELLVD_DDMA_IRQ);
#endif
}
/*****************************************************************************/
/*
 * p[0]: argcount==nr_of_branches
 * p[1]: flags (not defined yet)
 * p[2]: module            --> trigger id = 1<<0
 * p[3]: module            --> trigger id = 1<<1
 * ...                     --> ...          1<<2
 */
plerrcode
init_trig_pcilvd_async(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    ems_u32* pp;
    int i;

    printf("init_trig_pcilvd_async\n");

    priv=malloc(sizeof(struct private));
    if (!priv) {
        printf("init_trig_pcilvd_async: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->numbranches=p[0]-1; /* ignore p[1](=flags) */

    priv->flags=p[1];
    if (priv->numbranches<1) {
        free(priv);
        return plErr_ArgNum;
    }
    priv->data=malloc(priv->numbranches*sizeof(struct lvd_data));
    if (!priv->data) {
        free(priv);
        return plErr_NoMem;
    }

    pp=p+2;
    for (i=0; i<priv->numbranches; i++) {
        if (!valid_module(pp[0], modul_lvd)) {
            *outptr++=i;
            free(priv->data);
            free(priv);
            return plErr_ArgRange;
        }
        if (ModulEnt(pp[0])->address.lvd.dev->lvdtype!=lvd_sis1100) {
            printf("init_trig_pcilvd_async: wrong device type %d\n",
                    ModulEnt(pp[0])->address.lvd.dev->lvdtype);
            free(priv->data);
            free(priv);
            return plErr_ArgRange;
        }
        priv->data[i].module_idx=pp[0];
        priv->data[i].module=ModulEnt(pp[0]);
        priv->data[i].dev=priv->data[i].module->address.lvd.dev;
        priv->data[i].dev_info=
                (struct lvd_sis1100_info*)priv->data[i].dev->info;
        pp++;
    }

    trinfo->count=0;

    tinfo->insert_triggertask=insert_trig_pcilvd_async;
    tinfo->suspend_triggertask=suspend_trig_pcilvd_async;
    tinfo->reactivate_triggertask=reactivate_trig_pcilvd_async;
    tinfo->remove_triggertask=remove_trig_pcilvd_async;
#if 0
    tinfo->get_trigger=get_trig_pcilvd_async;
#endif
    tinfo->reset_trigger=reset_trig_pcilvd_async;
    tinfo->done_trigger=done_trig_pcilvd_async;

    tinfo->proctype=tproc_callback;

    return plOK;
}
/*****************************************************************************/
char name_trig_pcilvd_async[]="pcilvd_async";
int ver_trig_pcilvd_async=1;
/*****************************************************************************/
/*****************************************************************************/
