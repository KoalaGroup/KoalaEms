/*
 * trigger/pci/lvd/lvdtrigger.c
 * created 19.Jan.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvdtrigger.c,v 1.31 2016/05/12 22:00:12 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "../../../lowlevel/lvd/lvdbus.h"
#include "../../../lowlevel/lvd/lvd_access.h"
//#include "../../../lowlevel/lvd/sis1100/sis1100_lvd.h"

RCS_REGISTER(cvsid, "trigger/pci/lvd")

struct lvd_data {
    ems_u32 module_idx;
    ml_entry* module;
    struct lvd_dev* dev;
    struct lvd_sis1100_info* dev_info;
    ems_u32 mask;
    /*ems_u32 evnr;*/
};

struct private {
    int numbranches;
    struct lvd_data* data;
    int flags;
};

extern ems_u32* outptr;
extern int *memberlist;

/*****************************************************************************/
static void
trig_pcilvd_callback(struct lvd_dev *dev,
    const struct lvdirq_callbackdata *lvd_data,
    void* data)
{
    struct triggerinfo* trinfo=(struct triggerinfo*)data;
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct lvd_data *dat=0;
    int branch;

    trinfo->trigger=1<<31;
    for (branch=0; branch<priv->numbranches; branch++) {
        if (priv->data[branch].dev==dev) {
            dat=priv->data+branch;
            trinfo->trigger=1<<branch;
            break;
        }
    }

#if 0
    printf("[ev=%d] trig_pcilvd_callback mask=0x%x data=%p priv=%p\n",
            trigger.eventcnt, dat->mask, data, priv);
    if (dat->mask!=0x100)
        printf("trig_pcilvd_callback mask=0x%x\n", dat->mask);
#endif

    trinfo->time=lvd_data->time;
    trinfo->time_valid=1;

/* temporaere Loesung fuer falsche Interrupts */
{
    ems_u32 status;
    lvd_cc_r(dev, sr, &status);
    if (!(status&LVD_MSR_DATA_AV)) {
        dev->acknowledge_lvdirq(dev, dat->mask);
        printf("trig_pcilvd_callback(%s): no data, event %d, local count %d\n",
                dev->pathname, global_evc.ev_count, trinfo->count);
        return;
    }
}

    if (!(priv->flags&0x2)) {
#if 1
        lvd_cc_r(dev, event_nr, &trinfo->count);
#else
        {
            ems_u32 evc;
            lvd_cc_r(dev, event_nr, &evc);
            if (evc!=trinfo->eventcnt+1) {
                printf("evc: %d --> %d\n", trinfo->count, evc);
            }
            trinfo->count=evc;
        }
#endif
    } else {
        trinfo->count++;
    }

    trinfo->cb_proc(trinfo->cb_data);
}
/*****************************************************************************/
static void
remove_trig_pcilvd(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("remove_trig_pcilvd\n");
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;

        dev->unregister_lvdirq(dev, priv->data[i].mask, trig_pcilvd_callback);
    }
}
/*****************************************************************************/
static void
suspend_trig_pcilvd(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("suspend_trig_pcilvd, event=%d local count=%d\n",
            global_evc.ev_count, trinfo->count);
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;

        dev->unregister_lvdirq(dev, priv->data[i].mask, trig_pcilvd_callback);
    }
}
/*****************************************************************************/
static void
reactivate_trig_pcilvd(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("reactivate_trig_pcilvd, event=%d local count=%d\n",
            global_evc.ev_count, trinfo->count);
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;

        dev->register_lvdirq(dev, priv->data[i].mask, 0, trig_pcilvd_callback,
                trinfo, "trig_pcilvd_callback", 1);
        /*XXX hier muesste ein ignorierter IRQ nochmal aktiviert werden*/
    }
}
/*****************************************************************************/
static errcode
insert_trig_pcilvd(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("insert_trig_pcilvd\n");
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct lvd_dev* dev=priv->data[i].dev;
        ems_u32 cr;

        printf("trig_pcilvd[%d]: request IRQs 0x%x\n", i, priv->data[i].mask);
        printf("  installing trig_pcilvd_callback %p data=%p\n",
                trig_pcilvd_callback, trinfo);
        if (dev->register_lvdirq(dev, priv->data[i].mask, 0, trig_pcilvd_callback,
                trinfo, "trig_pcilvd_callback", 0)<0)
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
done_trig_pcilvd(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

#if 1
    printf("done_trig_pcilvd\n");
#endif
    free(priv->data);
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
#if 0
static void
read_idiotic_data(struct lvd_dev* dev)
{
    struct sis1100_vme_block_req breq;
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    int p=link->p_remote;
    ems_u32 buf[32];
    int n, i;

    breq.size=4;
    breq.fifo=1;
    breq.num=32;
    breq.am=-1;
    breq.addr=ofs(struct lvd_bus1100, prim_controller.cc.ro_data);
    breq.data=(u_int8_t*)buf;
    if (ioctl(p, SIS3100_VME_BLOCK_READ, &breq)) {
        printf("read_idiotic_data: %s\n", strerror(errno));
        return;
    }
    if (breq.error) {
        printf("read_idiotic_data: error=0x%x num=%llu\n",
                breq.error, (unsigned long long)breq.num);
    }
    n=breq.num;
    printf("breq.num=%d\n", n);
    for (i=0; i<n; i++) {
        printf("[%2d] %08x\n", i, buf[i]);
    }
}
#endif
/*****************************************************************************/
static void
reset_trig_pcilvd(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct lvd_data* dat;
    struct lvd_dev* dev;
    int branch, trigg=trinfo->trigger;

#if 0
    printf("[ev=%d] reset_trig_pcilvd\n", trigger.eventcnt);
#endif
    branch=0;
    while (!(trigg&1)) {
        branch++;
        trigg>>=1;
    }
    dat=priv->data+branch;
    dev=dat->dev;

    dev->acknowledge_lvdirq(dev, dat->mask);
    if (priv->flags&1) {
#if 0
        printf("reset_trig_pcilvd: set LVD_MSR_READY\n");
#endif
        lvd_cc_w(dev, sr, LVD_MSR_READY);
    }
}
/*****************************************************************************/
/*
 * p[0]: argcount==2*nr_of_branches
 * p[1]: flags
 *       0x1: handshake mode of controller (DAQ_HANDSH)
 *       0x2: use software event counter
 * p[2]: module            --> trigger id = 1<<0
 * p[3]: mask (==doorbell)
 * p[4]: module            --> trigger id = 1<<1
 * p[5]: mask (==doorbell)
 * ...                     --> ...        1<<2
 */
plerrcode
init_trig_pcilvd(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    ems_u32* pp;
    int i, nargs;

    printf("init_trig_pcilvd\n");

    nargs=p[0]-1; /* ignore p[1](=flags) */
    if (nargs&1)
        return plErr_ArgNum;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_pcilvd: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->flags=p[1];

    priv->numbranches=nargs/2;
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
            printf("init_trig_pcilvd: wrong device type %d\n",
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
        priv->data[i].mask=pp[1];
        pp+=2;
    }

    trinfo->count=0;

    tinfo->insert_triggertask=insert_trig_pcilvd;
    tinfo->suspend_triggertask=suspend_trig_pcilvd; /* suspend==remove?? */
    tinfo->reactivate_triggertask=reactivate_trig_pcilvd;
    tinfo->remove_triggertask=remove_trig_pcilvd;
    tinfo->get_trigger=0;
    tinfo->reset_trigger=reset_trig_pcilvd;
    tinfo->done_trigger=done_trig_pcilvd;

    tinfo->proctype=tproc_callback;

    return plOK;
}
/*****************************************************************************/
char name_trig_pcilvd[]="pcilvd";
int ver_trig_pcilvd=1;
/*****************************************************************************/
/*****************************************************************************/
