/*
 * trigger/general/periodictrigger.c
 * created 2007-May-21 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: periodictrigger.c,v 1.5 2013/12/06 20:28:14 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errorcodes.h>
#ifdef OBJ_VAR
#include "../../objects/var/variables.h"
#endif
#include <rcs_ids.h>
#include "../../main/scheduler.h"
#include "../trigger.h"
#include "../trigprocs.h"
#include "../../objects/pi/readout.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct period {
    struct triggerinfo* trinfo;
    ems_u32 trigger;
    unsigned period;
    unsigned int var_idx;
    ems_u32 *var_ptr;
    struct taskdescr *taskdescr;
};

struct private {
    int num_periods;
    struct period *periods;
};

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "trigger/general")

/*****************************************************************************/
static void
trig_periodic_callback(union callbackdata data)
{
    struct period *period=(struct period*)data.p;
    struct triggerinfo* trinfo=period->trinfo;

    trinfo->eventcnt++;
    trinfo->trigger=period->trigger;
    trinfo->cb_proc(trinfo->cb_data);
}
/*****************************************************************************/
static void
suspend_trig_periodic(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("suspend_trig_periodic\n");
#endif

    for (i=0; i<priv->num_periods; i++) {
        struct period *per=priv->periods+i;
        if (per->taskdescr) {
            printf("suspend_trig_periodic: remove task %p\n", per->taskdescr);
            sched_remove_task(per->taskdescr);
            per->taskdescr=0;
        }
    }
}
/*****************************************************************************/
static void
remove_trig_periodic(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("remove_trig_periodic\n");
#endif

    for (i=0; i<priv->num_periods; i++) {
        struct period *per=priv->periods+i;
        if (per->taskdescr) {
            printf("remove_trig_periodic: remove task %p\n", per->taskdescr);
            sched_remove_task(per->taskdescr);
            per->taskdescr=0;
        }
    }
}
/*****************************************************************************/
static void
reactivate_trig_periodic(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
printf("reactivate_trig_periodic\n");
#endif

    for (i=0; i<priv->num_periods; i++) {
        struct period *per=priv->periods+i;
        union callbackdata data;

        data.p=priv->periods+i;
        per->taskdescr=sched_exec_periodic(trig_periodic_callback,
                data, per->period, "periodic trigger");
        printf("reactivate_trig_periodic: got task %p\n", per->taskdescr);
        if (per->taskdescr==0) {
            printf("reactivate_trig_periodic: sched_exec_periodic failed\n");
        }
    }
}
/*****************************************************************************/
static errcode
insert_trig_periodic(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    int i;

#if 1
    printf("insert_trig_periodic\n");
#endif

    for (i=0; i<priv->num_periods; i++) {
        struct period *per=priv->periods+i;
        union callbackdata data;

        data.p=priv->periods+i;
        per->taskdescr=sched_exec_periodic(trig_periodic_callback,
                data, per->period, "periodic trigger");
        printf("insert_trig_periodic: got task %p\n", per->taskdescr);
        if (per->taskdescr==0) {
            printf("insert_trig_periodic: sched_exec_periodic failed\n");
            return Err_System;
        }
    }
    return OK;
}
/*****************************************************************************/
static plerrcode
done_trig_periodic(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    free(priv->periods);
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
#if 0
/*
 * get_trig_periodic will never be called
 * (all work is done in trig_periodic_callback)
 */
static int
get_trig_periodic(struct trigprocinfo* info)
{
    printf("!!! get_trig_periodic called (but it should not)!!!\n");
    return -1;
}
#endif
/*****************************************************************************/
#if 1
static void
reset_trig_periodic(struct triggerinfo* trinfo)
{
/*printf("reset_trig_periodic\n");*/
    /* do nothing */
}
#endif
/*****************************************************************************/
/*
 * p[0] number of arguments
 * p[1] trigger id
 * p[2] period/.01s
 * p[3] variable for period
 * [p[4..6] id/period/var for 2nd period
 * ...
 */

plerrcode init_trig_periodic(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    int i;

    if (p[0]<2)
        return plErr_ArgNum;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_periodic: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->num_periods=(p[0]+1)/3; /* last variable id is optional! */
    if ((p[0]!=priv->num_periods*3) && (p[0]!=priv->num_periods*3-1)) {
        free(priv);
        return plErr_ArgNum;
    }

    priv->periods=malloc(priv->num_periods*sizeof(struct period));
    if (!priv->periods) {
        free(priv);
        return plErr_NoMem;
    }

    for (i=0; i<priv->num_periods; i++) {
        struct period *per=priv->periods+i;

        per->trinfo=trinfo;
        per->taskdescr=0;
        per->trigger=p[3*i+1];
        per->period=p[3*i+2];
        if ((p[0]>3*i+3)&&(p[3*i+3]>0)) {
#ifdef OBJ_VAR
            unsigned int size;
            plerrcode pres;
            per->var_idx=p[3*i+3];
            pres=var_attrib(per->var_idx, &size);
            if (pres==plErr_NoVar) {
                size=1;
                pres=var_create(per->var_idx, size);
                if (pres==plOK)
                    var_write_int(per->var_idx, 0);
            }
            if (pres!=plOK) {
                free(priv->periods);
                free(priv);
                return pres;
            }

            if (size!=1) {
                free(priv->periods);
                free(priv);
                return plErr_IllVarSize;
            }

            var_get_ptr(per->var_idx, &(per->var_ptr));
            if (per->period>=0)
                *(per->var_ptr)=per->period;
            else
                per->period=*(per->var_ptr);
#else
            free(priv->periods);
            free(priv);
            return plErr_IllVar;
#endif
        } else {
            per->var_idx=0;
            per->var_ptr=0;
        }
        if (per->period<=0) {
            printf("init_trig_periodic: illegal period %d (i=%d)\n",
                    per->period, i);
            free(priv->periods);
            free(priv);
            return plErr_ArgRange;
        }
        if (per->trigger>=MAX_TRIGGER) {
            printf("init_trig_periodic: illegal trigger %d (i=%d)\n",
                    per->trigger, i);
            *outptr++=MAX_TRIGGER-1;
            free(priv->periods);
            free(priv);
            return plErr_ArgRange;
        }
    }

    trinfo->eventcnt=0;

    tinfo->insert_triggertask=insert_trig_periodic;
    tinfo->suspend_triggertask=suspend_trig_periodic;
    tinfo->reactivate_triggertask=reactivate_trig_periodic;
    tinfo->remove_triggertask=remove_trig_periodic;
    /*tinfo->get_trigger=get_trig_periodic;*/
    tinfo->reset_trigger=reset_trig_periodic;
    tinfo->done_trigger=done_trig_periodic;

    tinfo->proctype=tproc_callback;

    return plOK;
}
/*****************************************************************************/

char name_trig_periodic[]="periodic";
int ver_trig_periodic=1;

/*****************************************************************************/
/*****************************************************************************/

