/*
 * trigger/camac/qtrigger.c
 * created 01.04.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: qtrigger.c,v 1.16 2017/10/21 22:06:13 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/camac/camac.h"
#include "../../objects/domain/dom_ml.h"
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"

struct private {
    camadr_t addr;
    struct camac_dev* dev;
};

extern unsigned int* memberlist;

RCS_REGISTER(cvsid, "trigger/camac")

/*****************************************************************************/
static plerrcode
done_trig_q(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}

static int
get_trig_q(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    ems_u32 qx;

    priv->dev->CAMACread(priv->dev, &priv->addr, &qx);
    if (priv->dev->CAMACgotQ(qx)) {
        trinfo->count++;
        return 1;
    }
    return 0;
}

static void
reset_trig_q(__attribute__((unused)) struct triggerinfo* trinfo)
{}


plerrcode init_trig_q(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    ml_entry* m;
    if (*p!=3)
        return plErr_ArgNum;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_q: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    m=ModulEnt(p[1]);
    priv->dev=m->address.camac.dev;
    priv->addr=CAMACaddrM(m, p[2], p[3]);
    trinfo->count=0;

    tinfo->get_trigger=get_trig_q;
    tinfo->reset_trigger=reset_trig_q;
    tinfo->done_trigger=done_trig_q;

    tinfo->proctype=tproc_poll;

    return plOK;
}

char name_trig_q[]="Q-Response";
int ver_trig_q=1;

/*****************************************************************************/
/*****************************************************************************/
