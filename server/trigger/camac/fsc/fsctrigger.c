/*
 * trigger/camac/fsc/fsctrigger.c
 * created 01.04.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fsctrigger.c,v 1.13 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"

struct private {
    camadr_t fscaddr1, fscaddr2, fsccntl;
    int lastmem, singleevent;
    struct camac_dev* dev;
};

extern int *memberlist;

RCS_REGISTER(cvsid, "trigger/camac/fsc")

/*****************************************************************************/

static plerrcode
done_trig_fsc(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}

static int
get_trig_fsc(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct camac_dev* dev=priv->dev;
    ems_u32 qx;

    T(get_trig_fsc)

    if (priv->lastmem) {
        dev->CAMACread(dev, &priv->fscaddr1, &qx);
        if (dev->CAMACgotQ(qx)) {
            dev->CAMACcntl(dev, &priv->fscaddr1, &qx);
            trinfo->eventcnt++;
            priv->lastmem=0;
            return 1;
        }
    } else {
        dev->CAMACread(dev, &priv->fscaddr2, &qx);
        if (dev->CAMACgotQ(qx)) {
            dev->CAMACcntl(dev, &priv->fscaddr2, &qx);
            trinfo->eventcnt++;
            priv->lastmem=1;
            return 2;
        }
    }
    return 0;
}

static void
reset_trig_fsc(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct camac_dev* dev=priv->dev;

    if (priv->singleevent)
	dev->CAMACwrite(dev, &priv->fsccntl, priv->lastmem?0x25:0x23);
    else
	dev->CAMACwrite(dev, &priv->fsccntl, priv->lastmem?0x05:0x03);
}

plerrcode init_trig_fsc(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    ml_entry* m;

    T(init_trig_fsc)

    if (p[0]!=2)
            return plErr_ArgNum;
    if (p[1]>30)
            return plErr_BadHWAddr;

    m=ModulEnt(p[1]);
    if (m->modultype!=FSC)
            return plErr_BadModTyp;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_fsc: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->dev=m->address.camac.dev;
    D(D_TRIG, printf("FSC in Slot %d\n", p[1]);)
    priv->fscaddr1=CAMACaddrM(m, 0, 10);
    priv->fscaddr2=CAMACaddrM(m, 1, 10);
    priv->fsccntl=CAMACaddrM(m, 0, 16);
    priv->singleevent=p[2];
    trinfo->eventcnt=0;
    priv->lastmem=1;

    tinfo->get_trigger=get_trig_fsc;
    tinfo->reset_trigger=reset_trig_fsc;
    tinfo->done_trigger=done_trig_fsc;

    tinfo->proctype=tproc_poll;

    return plOK;
}

char name_trig_fsc[]="FSC";
int ver_trig_fsc=1;

/*****************************************************************************/
/*****************************************************************************/
