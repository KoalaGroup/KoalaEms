/*
 * trigger/vme/sis3316_poll.c
 * created: 2016-11-11 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3316_poll.c,v 1.2 2017/10/23 00:08:57 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/pi/readout.h"
#include "../../../main/scheduler.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../procs/unixvme/sis3316/sis3316.h"

struct private {
    int trigger;
    int bitmask; /* if given as 0: set to (1<<19)+(1<<21) */
    ml_entry* module;
    struct sis3316_priv_data *mpriv;
    int polled;
};

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "trigger/vme")

/*
 * trig_sis3316_poll polls the "Acquisition control/status register" of one
 * sis3316 module. It will work with VME and UDP, but in case of VME
 * it is better to use VME interrups instead of polling.
 * It checks if any bit given in bitmask is set in this register;
 * If bitmask is empty bit 19 (Memory Address Threshold flag) is used.
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
    ems_u32 reg60;
    plerrcode pres;

//printf("get_trig_sis3316_poll: n=%d\n", tpriv->polled++);

    pres=tpriv->mpriv->sis3316_read_reg(tpriv->module, 0x60, &reg60);
    if (pres!=plOK) {
        complain("trig_sis3316_poll(ser. %d): read_reg failed",
                tpriv->mpriv->serial);
        fatal_readout_error();
        return 0;
    }

/*
 * printf("reg60=%08x mask=%08x\n", reg60, tpriv->bitmask);
 * if (reg60==0xff0f8500)
 * exit(0);
 */
    if (reg60 & tpriv->bitmask) {
        //tpriv->mpriv->last_acq_state=reg60;
        //tpriv->mpriv->last_acq_state_valid=1;
        trinfo->count++;
        return tpriv->trigger;
    }

    return 0;
}
/*****************************************************************************/
static void reset_trig_sis3316_poll(
        __attribute__((unused)) struct triggerinfo* trinfo)
{
    /* nothing to do */
}
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: trigger
 * p[2]: module
 * p[3]: bitmask
 */
plerrcode
init_trig_sis3316_poll(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* tpriv;
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=1;
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }

    pres=get_modent(p[2], &module);
    if (pres!=plOK) {
        complain("init_trig_sis3316_poll: module %d not valid", p[2]);
        return pres;
    }
    if ((module->modulclass!=modul_vme && module->modulclass!=modul_ip)
                || module->modultype!=SIS_3316) {
        complain("init_trig_sis3316_poll: module %d is not a sis3316", p[2]);
        return plErr_BadModTyp;
    }

    tpriv=calloc(1, sizeof(struct private));
    if (!tpriv) {
        printf("init_trig_sis3316_poll: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=tpriv;
    tpriv->mpriv=(struct sis3316_priv_data*)module->private_data;
    tpriv->mpriv->last_acq_state_valid=0;

    tpriv->trigger=p[1];
    tpriv->module=ModulEnt(p[2]);
    tpriv->bitmask=p[3]?p[3]:(1<<19)+(1<<21);

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
