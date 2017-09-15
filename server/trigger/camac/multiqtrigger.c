/*
 * trigger/camac/multiqtrigger.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: multiqtrigger.c,v 1.15 2011/04/06 20:30:35 wuestner Exp $";

#if defined(__unix__)
#include <stdlib.h>
#endif
#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/camac/camac.h"
#include "../../objects/domain/dom_ml.h"
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"

struct qtrigger_addr {
    camadr_t addr;
    struct camac_dev* dev;
};

struct private {
    int triggeranz, nochzuliefern;
    int* wohin;
    struct qtrigger_addr* qqtrigger;
};

extern int* memberlist;

RCS_REGISTER(cvsid, "trigger/camac")

/*****************************************************************************/
static plerrcode
done_trig_qq(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    if (priv->qqtrigger)
        free(priv->qqtrigger);
    if (priv->wohin)
        free(priv->wohin);
    free(priv);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int
get_trig_qq(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

    if (!priv->nochzuliefern) {
	i=priv->triggeranz;
	while (i--) {
            struct qtrigger_addr* trig=priv->qqtrigger+i;
            ems_u32 qx;
            trig->dev->CAMACread(trig->dev, &trig->addr, &qx);
	    if (trig->dev->CAMACgotQ(qx)) {
		priv->wohin[priv->nochzuliefern]=i+1;
		priv->nochzuliefern++;
	    }
	}
    }
    if (priv->nochzuliefern) {
	trinfo->eventcnt++;
	return priv->wohin[--priv->nochzuliefern];
    }
    return 0;
}
/*****************************************************************************/
static void
reset_trig_qq(struct triggerinfo* trinfo)
{}
/*****************************************************************************/
plerrcode init_trig_qq(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    int i;

    if (p[0]<1)
            return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER-1)
            return plErr_ArgRange;
    if (p[0]!=p[1]*3+1)
            return plErr_ArgNum;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_qq: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->triggeranz=p[1];
    priv->qqtrigger=(struct qtrigger_addr*)calloc(priv->triggeranz,
            sizeof(struct qtrigger_addr));
    if (!priv->qqtrigger) {
        free(priv);
        return plErr_NoMem;
    }
    priv->wohin=calloc(priv->triggeranz, sizeof(int));
    if (!priv->wohin) {
        free(priv->qqtrigger);
        free(priv);
        return plErr_NoMem;
    }
    for (i=0; i<priv->triggeranz; i++) {
        ml_entry* m=ModulEnt(p[3*i+2]);
        priv->qqtrigger[i].addr=CAMACaddrM(m, p[3*i+3], p[3*i+4]);
        priv->qqtrigger[i].dev=m->address.camac.dev;
    }
    trinfo->eventcnt=0;
    priv->nochzuliefern=0;

    tinfo->get_trigger=get_trig_qq;
    tinfo->reset_trigger=reset_trig_qq;
    tinfo->done_trigger=done_trig_qq;

    tinfo->proctype=tproc_poll;

    return plOK;
}
/*****************************************************************************/
char name_trig_qq[]="Multi-Q-Response";
int ver_trig_qq=1;
/*****************************************************************************/
/*****************************************************************************/
