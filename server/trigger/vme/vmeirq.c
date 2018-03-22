/*
 * vmeirq Trigger
 * created: 2005-12-14 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vmeirq.c,v 1.9 2017/10/21 22:40:41 wuestner Exp $";

#include <stdio.h>

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#include "../../objects/pi/readout.h"
#include "dev/pci/sis1100_var.h"

#define DEBUG_TRIGG_VMEIRQ 0

struct vmeirq_private {
    int trigger;
    ml_entry* module;
    struct vme_dev* dev;
    struct vme_sis_info* dev_info;
    ems_u32 mask, vector, level;
};

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "trigger/vme")

/*
 * p[0]: argcount==4
 * p[1]: trigger
 * p[2]: module
 * p[3]: level
 * p[4]: vector
 */

/*****************************************************************************/
static void
#if 0
trig_vmeirq_callback(struct vme_dev* dev, ems_u32 mask,
        int level, ems_u32 vector, void* data)
#endif
trig_vmeirq_callback(__attribute__((unused)) struct vme_dev *dev,
        const struct vmeirq_callbackdata *vme_data,
        void* data)
{
    struct triggerinfo* trinfo=(struct triggerinfo*)data;
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct vmeirq_private* priv=(struct vmeirq_private*)tinfo->private;
    trinfo->count++;
    trinfo->trigger=priv->trigger;
    trinfo->time=vme_data->time;
    trinfo->time_valid=1;

#if DEBUG_TRIGG_VMEIRQ
    printf("trig_vmeirq_callback: calling cb_proc %p\n", trinfo->cb_proc);
#endif

    trinfo->cb_proc(trinfo->cb_data);
}
/*****************************************************************************/
static void
remove_trig_vmeirq(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct vmeirq_private* priv=(struct vmeirq_private*)tinfo->private;
    priv->dev->unregister_vmeirq(priv->dev, priv->mask, priv->vector);
}
/*****************************************************************************/
static void
reactivate_trig_vmeirq(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct vmeirq_private* priv=(struct vmeirq_private*)tinfo->private;
    priv->dev->register_vmeirq(priv->dev, priv->mask, priv->vector,
            trig_vmeirq_callback, trinfo);
}
/*****************************************************************************/
static errcode
insert_trig_vmeirq(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct vmeirq_private* priv=(struct vmeirq_private*)tinfo->private;

printf("insert_trig_vmeirq: callback=%p\n", trig_vmeirq_callback);

    if (priv->dev->register_vmeirq(priv->dev, priv->mask, priv->vector,
            trig_vmeirq_callback, trinfo)<0)
        return Err_System;
    return OK;
}
/*****************************************************************************/
static plerrcode
done_trig_vmeirq(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
#if 0
/*
 * get_trig_vmeirq will never be called
 * (all work is done in trig_vmeirq_callback)
 */
static int
get_trig_vmeirq(struct triggerinfo* trinfo)
{
    printf("!!! get_trig_vmeirq called (but it should not)!!!\n");
    return -1;
}
#endif
/*****************************************************************************/
static void
reset_trig_vmeirq(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct vmeirq_private* priv=(struct vmeirq_private*)tinfo->private;
    priv->dev->acknowledge_vmeirq(priv->dev, priv->mask, priv->vector);
}
/*****************************************************************************/
plerrcode
init_trig_vmeirq(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct vmeirq_private* priv;

    printf("init_trig_vmeirq\n");
    if (p[0]<4)
        return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=1;
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }
    if (!valid_module(p[2], modul_vme)) {
        *outptr++=2;
        return plErr_ArgRange;
    }
    if ((p[3]<1) || (p[3]>7)) {
        *outptr++=3;
        return plErr_ArgRange;
    }
    if (p[4]>255) {
        *outptr++=4;
        return plErr_ArgRange;
    }

    priv=calloc(1, sizeof(struct vmeirq_private));
    if (!priv) {
        printf("init_trig_vmeirq: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->trigger=p[1];
    priv->level=p[3];
    priv->mask=1<<priv->level;
    priv->vector=p[4];

    priv->module=ModulEnt(p[2]);
    priv->dev=priv->module->address.vme.dev;
    priv->dev_info=(struct vme_sis_info*)priv->dev->info;

    trinfo->count=0;

    tinfo->insert_triggertask=insert_trig_vmeirq;
    tinfo->suspend_triggertask=remove_trig_vmeirq;
    tinfo->reactivate_triggertask=reactivate_trig_vmeirq;
    tinfo->remove_triggertask=remove_trig_vmeirq;
    tinfo->reset_trigger=reset_trig_vmeirq;
    tinfo->done_trigger=done_trig_vmeirq;

    tinfo->proctype=tproc_callback;

    return plOK;
}
/*****************************************************************************/

char name_trig_vmeirq[] = "vmeirq";
int ver_trig_vmeirq     = 1;

/*****************************************************************************/
/*****************************************************************************/
