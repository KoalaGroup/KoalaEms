/*
 * trigger/camac/lamtrigger.c
 * created 24.03.98
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lamtrigger.c,v 1.11 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/camac/camac.h"
#include "../../lowlevel/devices.h"
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"

struct private {
    int triggermask;
    struct camac_dev* dev;
};

RCS_REGISTER(cvsid, "trigger/camac")

/*****************************************************************************/
static plerrcode
done_trig_lam(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}

static int
get_trig_lam(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    ems_u32 lam;
    int res=0;

    priv->dev->CAMAClams(priv->dev, &lam);
    if (lam & priv->triggermask) {
        trinfo->eventcnt++;
        res = 1;
    }
    return res;
}

static void
reset_trig_lam(struct triggerinfo* trinfo)
{}

/*
 * p[0]: args(==2)
 * p[1]: crate
 * p[2]: mask
 */
plerrcode init_trig_lam(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    int crate;
    plerrcode pres;

    if (*p!=2)
        return plErr_ArgNum;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_lam: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    crate=p[1];
    if ((pres=find_camac_odevice(crate, &priv->dev))!=plOK) {
        printf("init_trig_lam: invalid camac crate %d\n", crate);
        free(priv);
        return pres;
    }
    priv->triggermask=p[2];
    trinfo->eventcnt=0;

    tinfo->get_trigger=get_trig_lam;
    tinfo->reset_trigger=reset_trig_lam;
    tinfo->done_trigger=done_trig_lam;

    tinfo->proctype=tproc_poll;

    return plOK;
}

char name_trig_lam[]="LAM";
int ver_trig_lam=1;

/*****************************************************************************/
/*****************************************************************************/

