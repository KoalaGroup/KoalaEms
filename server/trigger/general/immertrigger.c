/*
 * trigger/general/immertrigger.c
 * created before 15.04.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: immertrigger.c,v 1.15 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include "emsctypes.h"
#include <rcs_ids.h>
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"

struct private {
    int trigger;
};

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "trigger/general")

/*****************************************************************************/
static plerrcode
done_trig_immer(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int
get_trig_immer(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    trinfo->count++;
    return priv->trigger;
}
/*****************************************************************************/
static void
reset_trig_immer(__attribute__((unused)) struct triggerinfo* trinfo)
{}
/*****************************************************************************/
/*
 * p[0] number of arguments
 * p[1] trigger id
 * [p[2] period/.01s]
 */

plerrcode
init_trig_immer(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;

    if ((p[0]<1)||(p[0]>2))
        return plErr_ArgNum;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }

    priv=malloc(sizeof(struct private));
    if (!priv) {
        printf("init_trig_immer: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->trigger=p[1];
    trinfo->count=0;

    tinfo->get_trigger=get_trig_immer;
    tinfo->reset_trigger=reset_trig_immer;
    tinfo->done_trigger=done_trig_immer;

    if (p[0]>1) {
        tinfo->proctype=tproc_timer;
        tinfo->i.tp_timer.time=p[2];
    } else {
        tinfo->proctype=tproc_poll;
    }

    return plOK;
}
/*****************************************************************************/

char name_trig_immer[]="Immer";
int ver_trig_immer=1;

/*****************************************************************************/
/*****************************************************************************/
