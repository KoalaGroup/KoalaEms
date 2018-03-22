/*
 * trigger/camac/drams/dramstrigger.c
 * created 22.10.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dramstrigger.c,v 1.8 2016/05/12 22:00:12 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../objects/domain/dom_ml.h"

struct private {
    camadr_t addr;
    struct camac_dev* dev;
};

RCS_REGISTER(cvsid, "trigger/camac/drams")

/*****************************************************************************/
static plerrcode
done_trig_drams(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}

static int
get_trig_drams(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct camac_dev* dev=priv->dev;
    ems_u32 val;

    dev->CAMACread(dev, &priv->addr, &val);
    if ((val&3)==2) {
        trinfo->count++;
        return 1;
    }
    return 0;
}

static void
reset_trig_drams(struct triggerinfo* trinfo)
{}


plerrcode init_trig_drams(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!modullist || (p[1]>=modullist->modnum)) {
        return plErr_ArgRange;
    }

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_drams: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->dev=modullist->entry[p[1]].address.camac.dev;
    priv->addr=priv->dev->CAMACaddr(modullist->entry[p[1]].address.camac.slot,
            0, 0);
    trinfo->count=0;

    tinfo->get_trigger=get_trig_drams;
    tinfo->reset_trigger=reset_trig_drams;
    tinfo->done_trigger=done_trig_drams;

    tinfo->proctype=tproc_poll;

    return plOK;
}

char name_trig_drams[]="DRAMS";
int ver_trig_drams=1;

/*****************************************************************************/
/*****************************************************************************/

