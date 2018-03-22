/*
 * trigger/vme/sis3316_poll_multi.c
 * 2016-11-11 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3316_poll_multi.c,v 1.1 2016/11/26 01:26:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../objects/var/variables.h"
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../procs/unixvme/sis3316/sis3316.h

struct private {
    int trigger;
    int bitmask; /* if given as 0: set to (1<<19) */
    int nrmodules;
    ml_entry** modules;
    ems_u32* values;
};

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "trigger/vme")

/*
 * trig_sis3316_poll_multi is used to poll the
 * "Acquisition control/status register" (0x60) of more then one
 * sis3316 modules over UDP.
 * It checks if any bit given in bitmask is set in this register;
 * If bitmask is empty bit 19 (Memory Address Threshold flag) is used.
 *
 * trig_sis3316_poll_multi can be used only if:
 * - all modules are of type sis3316
 * - all modules are connected via IP
 * - all modules use the same local socket
 *
 * For mixed access (VME or different sockets) trig_sis3316mixed_poll can be
 * used.
 *
 * If the FP-bus is used to distribute the threshold information
 * one module only has to be checked. In this case trig_sis3316_poll
 * is the better choice.
 */
/*****************************************************************************/
static plerrcode
done_trig_sis3316_poll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int
get_trig_sis3316_poll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private *tpriv=(struct private*)tinfo->private;
    struct sis3316_priv_data *mpriv;
    plerrcode pres;

    pres=tpriv->modules[0]->sis3316_read_reg_multi(
            tpriv->modules, tpriv->values, tpriv->nrmodules);
    if (pres!=plOK) {
        complain("trig_sis3316_poll: read_reg_multi failed");
        fatal_readout_error();
        return 0;
    }

    /* check for threshold */
    trigger=0;
    for (i=0; i<tpriv->nrmodules; i++) {
        if (tpriv->values[i]&tpriv->bitmask)
            trigger=tpriv->trigger;
    }

    /* store register values (for use by readout procedure) */
    if (trigger) {
        tpriv->modules[i]->
        ((struct sis3316_priv_data*)mod->private_data)->
    }



    mpriv=(struct sis3316_priv_data*)module->private_data;

    pres=mpriv->sis3316_read_reg(tpriv->module, tpriv->offset, value);



           pres=sis3316_read_reg(module, 0x0, outptr);
            if (pres==plOK)
                outptr++;



    return priv->sis3316_read_reg(module, reg, value);










    struct vme_dev* dev=priv->dev;
    unsigned int temp = 0;
    unsigned short temp16 = 0;
    switch(priv->mode) {
    case 2:
        dev->read_a32d16(dev, priv->module->address.vme.addr+priv->offset,
                &temp16);
        temp=temp16;
        break;
    case 4:
        dev->read_a32d32(dev, priv->module->address.vme.addr+priv->offset,
                &temp);
        break;
    }

#if 0
    printf("VMEtrigger: temp = 0x%04hx  mask = 0x%04hx invert = 0x%04hx\n",
            temp, mask, invert);  
#endif

    if ((temp & priv->mask)==priv->invert) {
        trinfo->count++;
        return priv->trigger;
    }

    return 0;
}
/*****************************************************************************/
static void reset_trig_sis3316_poll(struct triggerinfo* trinfo)
{}
/*****************************************************************************/

/*
 * p[0]: argcount==5
 * p[1]: trigger
 * p[2]: threshold
 * p[3...]: modules
 */
plerrcode init_trig_sis3316_poll(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;

    if (p[0]!=4)
        return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_VMEpoll: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->trigger=p[1];
    priv->offset=p[3];
    priv->mask=p[4];

    priv->module=ModulEnt(p[2]);


    trinfo->count=0;	

    tinfo->get_trigger=get_trig_sis3316_poll;
    tinfo->reset_trigger=reset_trig_sis3316_poll;
    tinfo->done_trigger=done_trig_sis3316_poll;

    tinfo->proctype=tproc_poll;

    return plOK;
}
/*****************************************************************************/

char name_trig_sis3316_poll[] = "sis3316_poll";
int ver_trig_sis3316_poll     = 1;

/*****************************************************************************/
/*****************************************************************************/
