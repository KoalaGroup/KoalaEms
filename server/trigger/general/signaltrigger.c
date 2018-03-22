/*
 * trigger/general/signaltrigger.c
 * created 24.02.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: signaltrigger.c,v 1.18 2016/05/12 22:00:12 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#ifdef OBJ_VAR
#include "../../objects/var/variables.h"
#endif
#include "../../main/scheduler.h"
#include "../../main/signals.h"
#include "../trigger.h"
#include "../trigprocs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct private {
    int trigger;
    int siganz;
    int *signals;
};

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "trigger/general")

/*****************************************************************************/
#ifdef OBJ_VAR
#ifdef OSK
static void sighndlr(int s)
#else
static void sighndlr(union SigHdlArg arg, int s)
#endif
{
    struct triggerinfo* trinfo=(struct triggerinfo*)arg.ptr;
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

    for (i=0; i<priv->siganz; i++) {
        if ((s==priv->signals[i]) && !priv->trigger) {
            trinfo->count++;
            priv->trigger=i+1;
        }
    }
}
#endif
/*****************************************************************************/
static plerrcode done_trig_signal(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

    if (priv->signals) {
        for (i=0; i<priv->siganz; i++) {
            if (remove_signalhandler(priv->signals[i]))
                return Err_System;
        }
        free(priv->signals);
    }
    free(priv);
    tinfo->private=0;

    return plOK;
}

static int get_trig_signal(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    return priv->trigger;
}

static void reset_trig_signal(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    priv->trigger=0;
}

plerrcode init_trig_signal(ems_u32* p, struct triggerinfo* trinfo)
{
#ifndef OBJ_VAR
    return plErr_IllVar;
#else
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    int i;

    if (p[0]!=2)
        return plErr_ArgNum;
    if (p[2]>MAX_VAR)
        return plErr_IllVar;
    if (!var_list[p[2]].len)
        return plErr_NoVar;
    if (p[1]!=var_list[p[2]].len)
        return plErr_IllVarSize;
    if (p[1]>=MAX_TRIGGER) {
        *outptr++=MAX_TRIGGER-1;
        return plErr_ArgRange;
    }

    priv=malloc(sizeof(struct private));
    if (!priv) {
        printf("init_trig_signal: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->trigger=0;
    trinfo->count=0;
    priv->siganz=p[1];
    priv->signals=calloc(priv->siganz, sizeof(int));
    if (!priv->signals) {
        free(priv);
        return plErr_NoMem;
    }

    for (i=0; i<priv->siganz; i++) {
#ifdef OSK
        priv->signals[i]=install_signalhandler(sighndlr);
#else
        {
            union SigHdlArg a;
            a.ptr=trinfo;
            priv->signals[i]=install_signalhandler(sighndlr, a, "signaltrigger");
        }
#endif
        if (priv->signals[i]==-1) {
            int j;
            for (j=0;j<i;j++)
                remove_signalhandler(priv->signals[j]);
            free(priv->signals);
            free(priv);
            return plErr_Other;
        }
        if (priv->siganz==1)
            var_list[p[2]].var.val=priv->signals[i];
        else
            var_list[p[2]].var.ptr[i]=priv->signals[i];
    }

    tinfo->get_trigger=get_trig_signal;
    tinfo->reset_trigger=reset_trig_signal;
    tinfo->done_trigger=done_trig_signal;

    tinfo->proctype=tproc_signal;

    return plOK;
#endif
}

char name_trig_signal[]="Signaltrigger";
int ver_trig_signal=1;

/*****************************************************************************/
/*****************************************************************************/

