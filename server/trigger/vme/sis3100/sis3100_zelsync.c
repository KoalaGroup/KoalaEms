/*
 * trigger/vme/sis3100/sis3100_zelsync.c
 * created 2007-Mar-16 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_zelsync.c,v 1.23 2011/04/06 20:30:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <objecttypes.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../main/signals.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../commu/commu.h"
#include <dev/pci/sis1100_var.h>

struct sis3100data {
    int module_idx;
    ml_entry* module;
    ems_u32 addr; /* base address of synchronisation module */
    struct vme_dev* dev;
    struct vme_sis_info* dev_info;
    ems_u32 ident;
    ems_u32 flags;
    ems_u32 syncmask;
    ems_u32 sis3100mask;
};

struct private {
    int numbranches;
    struct sis3100data* data;
    int flags;
    int active_branch; /* needed for reset_trig_* */
};

extern ems_u32* outptr;
extern int *memberlist;

#define ofs(what, elem) ((int)&(((what *)0)->elem))

RCS_REGISTER(cvsid, "trigger/vme/sis3100")

/*****************************************************************************/
#if 0
static char*
getbits(unsigned int v)
{
    static char s[42];
    char st[32];
    int i, j, k, l, m;

    for (i=0; i<41; i++) s[i]=' ';
    s[41]=0;
    for (i=0; i<32; i++) st[i]='0';

    for (i=31; v; i--, v>>=1) if (v&1) st[i]='1';

    l=m=0;
    for (k=0; k<4; k++) {
        for (j=0; j<2; j++) {
            for (i=0; i<4; i++) s[l++]=st[m++];
            l++;
        }
        l++;
    }
    return s;
}
#endif
/*****************************************************************************/
static void
trig_sis3100_dump_registers(struct vme_dev *dev, struct sis3100data *dat)
{
    ems_u32 in_out, in_latch;
    ems_u32 ident, serial, cr, sr, evc, tpat;

    printf("trig_3100_sync register dump:\n");

    dev->read_controller(dev, 1, 0x80, &in_out);
    dev->read_controller(dev, 1, 0x84, &in_latch);

    dev->read_a32d32(dev, dat->addr+0x00, &ident);
    dev->read_a32d32(dev, dat->addr+0x04, &serial);
    dev->read_a32d32(dev, dat->addr+0x08, &cr);
    dev->read_a32d32(dev, dat->addr+0x0c, &sr);
    dev->read_a32d32(dev, dat->addr+0x10, &evc);
    dev->read_a32d32(dev, dat->addr+0x14, &tpat);

    printf("  sis3100 in_out   = 0x%08x\n", in_out);
    printf("  sis3100 in_latch = 0x%08x\n", in_latch);
    printf("  sync ident       = 0x%08x\n", ident);
    printf("  sync serial      = 0x%08x\n", serial);
    printf("  sync cr          = 0x%08x\n", cr);
    printf("  sync sr          = 0x%08x\n", sr);
    printf("  sync evc         = 0x%08x\n", evc);
    printf("  sync tpat        = 0x%08x\n", tpat);
}
/*****************************************************************************/
static void
trig_sis3100_zelsync_callback(struct vme_dev *dev,
        const struct vmeirq_callbackdata *vme_data,
        void* data)
{
    struct triggerinfo* trinfo=(struct triggerinfo*)data;
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct sis3100data *dat=0;
    ems_u32 sr, latchreg, old_evnr;
    int branch, cc;

#if 0
    printf("trig_sis3100_zelsync_callback(%s):  mask=0x%x\n",
        dev->pathname, vme_data->mask);
#endif

    trinfo->trigger=0;

    for (branch=0; branch<priv->numbranches; branch++) {
        if (priv->data[branch].dev==dev) {
            dat=priv->data+branch;
            priv->active_branch=branch; /* we save it for reset_trig_* */
            break;
        }
    }

    /* read latch register */
    if (dev->read_controller(dev, 1, 0x84, &latchreg)<0) {
        printf("trig_3100_sync(%s): cannot read latch register\n", dev->pathname);
        send_unsol_var(Unsol_RuntimeError, 3, rtErr_Trig, trinfo->eventcnt, 101);
        fatal_readout_error();
    }
    if (!(latchreg&(dat->sis3100mask<<8))) {
        /* this was only the falling edge of input signal */

/* und wenn da was falsch ist? */
        {
            int weiter=0;
            ems_u32 sr, evc;
            dev->read_a32d32(dev, dat->addr+0xc, &sr);
            dev->read_a32d32(dev, dat->addr+0x10, &evc);
            if (sr) {
//                printf("trig_3100_sync: falling edge but sr=%08x\n", sr);
                weiter=1;
            }
            if (evc>trinfo->eventcnt) {
//                printf("trig_3100_sync: falling edge but evc=%d, old=%d\n",
//                        evc, trinfo->eventcnt);
                weiter=1;
            }
            if (weiter)
                goto weiter;
        }
        dev->acknowledge_vmeirq(dev, dat->sis3100mask, 0);
        return;
    }

weiter:
    cc=10;
    do {
        dev->read_a32d32(dev, dat->addr+0xc, &sr);
    } while (!sr && --cc);

    if (!sr) {
        printf("trig_3100_sync: sr=0\n");
        trig_sis3100_dump_registers(dev, dat);
        dev->acknowledge_vmeirq(dev, dat->sis3100mask, 0);
        dev->write_a32d32(dev, dat->addr+0xc, 3);
	send_unsol_var(Unsol_RuntimeError, 3, rtErr_Trig, trinfo->eventcnt, 102);
        return;
    }

    if (sr!=1 && !(dat->flags&0x10)) {
        printf("trig_3100_sync(%s): sr=0x%x count=%d, evnr=%d\n",
            dev->pathname, sr, cc, trinfo->eventcnt);
        trig_sis3100_dump_registers(dev, dat);
        send_unsol_var(Unsol_RuntimeError, 3, rtErr_Trig, trinfo->eventcnt, 102);
        fatal_readout_error();
        dev->acknowledge_vmeirq(dev, dat->sis3100mask, 0);
        dev->write_a32d32(dev, dat->addr+0xc, 3);
        return;
    }

    old_evnr=trinfo->eventcnt;
    if (!(priv->flags&0x2)) {
        dev->read_a32d32(dev, dat->addr+0x10, &trinfo->eventcnt);
    } else {
        trinfo->eventcnt++;
    }

    if (dat->flags&0x1)
        dev->read_a32d32(dev, dat->addr+0x14, &trinfo->trigger);
    else
        trinfo->trigger=1<<branch;

    trinfo->time=vme_data->time;
    trinfo->time_valid=1;

    /* this is nonsense if eventcounter wraps over */
    if (old_evnr && trinfo->eventcnt<=old_evnr) {
        printf("trig_3100_sync(%s): illegal event count %d (after %d)\n",
                dev->pathname, trinfo->eventcnt, old_evnr);
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, old_evnr, 103,
                trinfo->eventcnt);
        //fatal_readout_error();
    }

#if 0
    printf("trig_sis3100_zelsync_callback: trigger=%d event %d\n",
        trinfo->trigger, trinfo->eventcnt);
    trig_sis3100_dump_registers(dev, dat);
#endif

    /* call the readout procedure */
    trinfo->cb_proc(trinfo->cb_data);
}
/*****************************************************************************/
static void
remove_trig_sis3100_zelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("remove_trig_sis3100_zelsync\n");
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct vme_dev* dev=priv->data[i].dev;

        dev->unregister_vmeirq(dev, priv->data[i].sis3100mask, 0);
    }
}
/*****************************************************************************/
static void
reactivate_trig_sis3100_zelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("reactivate_trig_sis3100_zelsync\n");
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct vme_dev* dev=priv->data[i].dev;

        dev->register_vmeirq(dev, priv->data[i].sis3100mask, 0,
                trig_sis3100_zelsync_callback, trinfo);
        /*XXX hier muesste ein ignorierter IRQ nochmal aktiviert werden*/
    }
}
/*****************************************************************************/
static errcode
insert_trig_sis3100_zelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("insert_trig_sis3100_zelsync trinfo=%p\n", trinfo);
#endif
    for (i=0; i<priv->numbranches; i++) {
        struct vme_dev* dev=priv->data[i].dev;

        printf("trig_sis3100_zelsync(%s): request IRQs 0x%x\n",
            priv->data[i].dev->pathname, priv->data[i].sis3100mask);

        if (dev->register_vmeirq(dev, priv->data[i].sis3100mask, 0,
                trig_sis3100_zelsync_callback, trinfo)<0)
            return Err_System;
    }
    return OK;
}
/*****************************************************************************/
static plerrcode
done_trig_sis3100_zelsync(struct triggerinfo* trinfo)
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
 * get_trig_sis3100 will never be called
 * (all work is done in trig_sis3100_zelsync_callback)
 */
static int
get_trig_sis3100_zelsync(struct triggerinfo* trinfo)
{
    printf("!!! get_trig_sis3100_zelsync called (but it should not)!!!\n");
    return -1;
}
#endif
/*****************************************************************************/
static void
reset_trig_sis3100_zelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct sis3100data* dat=priv->data+priv->active_branch;
    struct vme_dev* dev=dat->dev;

#if 0
    printf("reset_trig_sis3100_zelsync trinfo=%p\n", trinfo);
#endif
    dev->acknowledge_vmeirq(dev, dat->sis3100mask, 0);
    dev->write_a32d32(dev, dat->addr+0xc, 3);
}
/*****************************************************************************/
/*
 * p[0] argcount==1+4*nr_of_modules
 * p[1]: global flags (common for all branches)
 *       0x2: use software event counter
 * 
 * p[2]: module            --> trigger id = 1<<0
 * p[3]: local flags (individual for each branch); ORed with cr
 *       0x01: expect trigger pattern
 *       0x10: use _local_ hardware event counter
 *       0x20: trigger pulse on LEMO output
 *       0x40: trigger pulse on ECL output
 * p[4]: input mask for synch module
 *       bit 0: RJ45 (ECL)
 *       bit 1: Coax (ECL)
 *       bit 2: LEMO input (NIM)
 *       bit 3: optics
 * p[5]: input mask for sis3100
 *       IRQ bits of sis3100 register OPT-IN-LATCH_IRQ
 *       bit 0..3: flat input (ECL)
 *       bit 4..6: LEMO input (NIM)
 * 
 * p[6]: module            --> trigger id = 1<<1
 * p[7]: local flags
 * p[8]: mask
 * ...                     --> ...        1<<2
 */

plerrcode
init_trig_sis3100_zelsync(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    const int words_per_branch=4;
    ems_u32* pp;
    int i, nargs;

#if 1
    printf("init_trig_sis3100_zelsync\n");
#endif

    nargs=p[0]-1; /* ignore p[1](=flags) */
    if (nargs%words_per_branch)
        return plErr_ArgNum;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_sis3100_zelsync: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->flags=p[1];
    priv->numbranches=nargs/words_per_branch;
#if 1
    printf("init_trig_sis3100_zelsync: global flags=0x%x, numbranches=%d\n",
        p[1], priv->numbranches);
#endif
    if (priv->numbranches<1) {
        free(priv);
        return plErr_ArgNum;
    }
    priv->data=malloc(priv->numbranches*sizeof(struct sis3100data));
    if (!priv->data) {
        free(priv);
        return plErr_NoMem;
    }

    /* read arguments and fill struct sis3100data[] */
    pp=p+2;
    for (i=0; i<priv->numbranches; i++) {
        if (!valid_module(pp[0], modul_vme, 0)) {
            *outptr++=i;
            free(priv->data);
            free(priv);
            return plErr_ArgRange;
        }
        if (ModulEnt(pp[0])->address.vme.dev->vmetype!=vme_sis3100) {
            printf("init_trig_sis3100_zelsync: wrong device type %d\n",
                    ModulEnt(pp[0])->address.vme.dev->vmetype);
            free(priv->data);
            free(priv);
            return plErr_ArgRange;
        }
        priv->data[i].module_idx=pp[0];
        priv->data[i].module=ModulEnt(pp[0]);
        priv->data[i].addr=priv->data[i].module->address.vme.addr;
        priv->data[i].dev=priv->data[i].module->address.vme.dev;
        priv->data[i].dev_info=(struct vme_sis_info*)priv->data[i].dev->info;
        priv->data[i].flags=pp[1];
        priv->data[i].syncmask=pp[2]&0xf;
        priv->data[i].sis3100mask=(pp[3]&0x7f)<<8;
        pp+=words_per_branch;
#if 1
        printf("init_trig_sis3100_zelsync(%s): flags=0x%x, sis3100mask=0x%x "
                "syncmask=0x%x, addr=0x%08x\n",
            priv->data[i].dev->pathname, priv->data[i].flags,
            priv->data[i].sis3100mask,
            priv->data[i].syncmask,
            priv->data[i].addr);
#endif
    }

    /* initialize synch modules */
    for (i=0; i<priv->numbranches; i++) {
        struct sis3100data* pd=priv->data+i;
        ems_u32 cr, val;

        /* read ident and serial number */
        if (pd->dev->read_a32d32(pd->dev, pd->addr+0x0, &pd->ident)<0) {
            printf("init_trig_sis3100_zelsync: error reading ident\n");
            goto error;
        }
        printf("sis3100_zelsync[%d]: ident=%04x FW=%x.%x\n",
                i, pd->ident, (pd->ident>>4)&0xf, pd->ident&0xf);
        if (pd->dev->read_a32d32(pd->dev, pd->addr+0x4, &val)<0)
            printf("init_trig_sis3100_zelsync: error reading serial number\n");
        else
            printf("sis3100_zelsync[%d]: serial=%d\n", i, val&0xffff);
        if (pd->flags&0x1 && (pd->ident&0xf0)<0x10) {
            printf("sis3100_zelsync[%d]: trigger pattern not supported\n", i);
            goto error;
        }

        /* master reset */
        if (pd->dev->write_a32d32(pd->dev, pd->addr+0xc, 1<<15)<0) {
            printf("init_trig_sis3100_zelsync: error trying master reset\n");
            free(priv->data);
            free(priv);
            return plErr_System;
        }
        /* reset control register */
        if (pd->dev->write_a32d32(pd->dev, pd->addr+0x8, 0)<0) {
            printf("init_trig_sis3100_zelsync: error writing control register\n");
            free(priv->data);
            free(priv);
            return plErr_System;
        }
        /* clear event counter */
        if (pd->dev->write_a32d32(pd->dev,
                pd->addr+0x10, 0)<0) {
            printf("init_trig_sis3100_zelsync: error writing event counter\n");
            free(priv->data);
            free(priv);
            return plErr_System;
        }
        /* clear status */
        if (pd->dev->write_a32d32(pd->dev,
                pd->addr+0xc, 0x3)<0) {
            printf("init_trig_sis3100_zelsync: error writing status register\n");
            free(priv->data);
            free(priv);
            return plErr_System;
        }
        /* set control register */
        cr=pd->syncmask;
        if (pd->flags&0x01) /* use trigger pattern*/
            cr|=0x100;
        cr|=pd->flags&0x70;
        if (pd->dev->write_a32d32(pd->dev,
                pd->addr+0x8, cr)<0) {
            printf("init_trig_sis3100_zelsync: error writing control register\n");
            free(priv->data);
            free(priv);
            return plErr_System;
        }
    }

    trinfo->eventcnt=0;

    tinfo->insert_triggertask=insert_trig_sis3100_zelsync;
    tinfo->suspend_triggertask=remove_trig_sis3100_zelsync;
    tinfo->reactivate_triggertask=reactivate_trig_sis3100_zelsync;
    tinfo->remove_triggertask=remove_trig_sis3100_zelsync;
    tinfo->reset_trigger=reset_trig_sis3100_zelsync;
    tinfo->done_trigger=done_trig_sis3100_zelsync;

    tinfo->proctype=tproc_callback;

    return plOK;

error:
    free(priv->data);
    free(priv);
    return plErr_System;
}
/*****************************************************************************/
char name_trig_sis3100_zelsync[]="sis3100_zelsync";
int ver_trig_sis3100_zelsync=1;
/*****************************************************************************/
/*****************************************************************************/
