/*
 * c219.c
 * created 17.01.95 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: c219.c,v 1.8 2016/05/12 22:00:12 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../objects/domain/dom_ml.h"

struct private {
    struct camac_dev* dev;
    int trigger, slot, kanal;
};

RCS_REGISTER(cvsid, "trigger/camac")

/*****************************************************************************/
static plerrcode
done_trig_c219(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct camac_dev* dev=priv->dev;

    dev->CAMACwrite(dev, dev->CAMACaddrP(priv->slot, 0, 16), 0);
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}

static int
get_trig_c219(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct camac_dev* dev=priv->dev;

    /* output, positive, normal, transparent */
    dev->CAMACwrite(dev, dev->CAMACaddrP(priv->slot, priv->kanal, 17), 6);
    dev->CAMACwrite(dev, dev->CAMACaddrP(priv->slot, 0, 16), 1<<priv->kanal);
    trinfo->count++;
    return priv->trigger;
}

static void
reset_trig_c219(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct camac_dev* dev=priv->dev;

    dev->CAMACwrite(dev, dev->CAMACaddrP(priv->slot, 0, 16), 0);
}

/*
p[0] : argnum
p[1] : index in modullist
p[2] : kanal
p[3] : id
*/
plerrcode init_trig_c219(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;

    if (p[0]!=3)
        return plErr_ArgNum;

    if (!modullist || (p[1]>=modullist->modnum)) {
        return plErr_ArgRange;
    }
    if (modullist->entry[p[1]].modultype!=CAEN_PROG_IO)
        return plErr_BadModTyp;
    if (p[2]>15)
        return plErr_ArgRange;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_c219: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->kanal=p[2];
    priv->trigger=p[3];
    trinfo->count=0;
    priv->dev=modullist->entry[p[1]].address.camac.dev;
    priv->slot=modullist->entry[p[1]].address.camac.slot;

    tinfo->get_trigger=get_trig_c219;
    tinfo->reset_trigger=reset_trig_c219;
    tinfo->done_trigger=done_trig_c219;

    tinfo->proctype=tproc_poll;

    return plOK;
}

char name_trig_c219[]="C219";
int ver_trig_c219=1;

/*****************************************************************************/
/*****************************************************************************/
