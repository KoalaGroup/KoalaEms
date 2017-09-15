/*
 * trigger/struct trigprocs.c.m4
 * created before 24.02.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: trigprocs.c.m4,v 1.18 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <xdrstring.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include "../objects/pi/readout.h"
#include "../main/server.h"
#include "../main/scheduler.h"
#include "trigger.h"
#include "trigprocs.h"
#include <rcs_ids.h>

static const char *trigtypenames[]={
    "other(??\?)",
    "select",
    "poll",
    "signal",
    "timer",
    "callback",
    "invalid"
};

RCS_REGISTER(cvsid, "trigger")

/*****************************************************************************/

define(`trig',`{
    init_trig_$1,
    name_trig_$1,
    &ver_trig_$1
},')

struct trigproc Trig[]= {
    include(procedures)
};

int NrOfTrigs=sizeof(Trig)/sizeof(struct trigproc);

struct triggerinfo trigger;
extern ems_u32* outptr;
extern int* memberlist;

/*****************************************************************************/
int
trigger_init(void)
{
    trigger.cb_proc=0;
    trigger.tinfo=0;
    return 0;
}
/*****************************************************************************/
int
trigger_done(void)
{
    trigger.cb_proc=0;
    trigger.tinfo=0;
    return 0;
}
/*****************************************************************************/
errcode
gettrigproclist(ems_u32* p, unsigned int num)
{
    register int i;

    T(gettrigproclist)
    D(D_REQ, printf("GetCapabilityList(Capab_trigproc)\n");)
    *outptr++=NrOfTrigs;
    for (i=0; i<NrOfTrigs; i++) {
        *outptr++=i;
        outptr=outstring(outptr, Trig[i].name_trig);
        *outptr++= *(Trig[i].ver_trig);
    }
    return OK;
}
/*****************************************************************************/
static void
trigger_Xtask(struct triggerinfo *trinfo)
{
    trinfo->time_valid=0;
    if ((trinfo->trigger=get_trigger(trinfo))==0)
        return;
    if (!trinfo->time_valid) {
        struct timeval now;
        gettimeofday(&now, 0);
        trinfo->time.tv_sec=now.tv_sec;
        trinfo->time.tv_nsec=now.tv_usec*1000;
    }
    trinfo->cb_proc(trinfo->cb_data);
}
/*****************************************************************************/
static void
trigger_task(union callbackdata data)
{
    return trigger_Xtask((struct triggerinfo*)data.p);
}
/*****************************************************************************/
static void
trigger_stask(int path, enum select_types types, union callbackdata data)
{
    return trigger_Xtask((struct triggerinfo*)data.p);
}
/*****************************************************************************/
static errcode insert_def_triggertask_select(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    union callbackdata calldata;
    calldata.p=trinfo;
    tinfo->i.tp_select.st=sched_insert_select_task(
                trigger_stask,
                calldata,
                "trigger",
                tinfo->i.tp_select.path,
                tinfo->i.tp_select.seltype
#ifdef SELECT_STATIST
                , 1
#endif
                );
    return tinfo->i.tp_select.st?OK:Err_System;
}
static errcode insert_def_triggertask_poll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    union callbackdata calldata;
    calldata.p=trinfo;
    tinfo->i.tp_poll.t=sched_insert_poll_task(trigger_task,
            calldata, READOUT_PRIOR, 0, "trigger");
    return tinfo->i.tp_poll.t?OK:Err_System;
}
static errcode insert_def_triggertask_signal(struct triggerinfo* trinfo)
{
    printf("insert_def_triggertask_signal: not yet implemented\n");
    return Err_Program;
}
static errcode insert_def_triggertask_timer(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    union callbackdata calldata;
    calldata.p=trinfo;
    tinfo->i.tp_timer.t=sched_exec_periodic(trigger_task, calldata,
            tinfo->i.tp_timer.time, "trigger");
    return tinfo->i.tp_timer.t?OK:Err_System;
}
/*---------------------------------------------------------------------------*/
static void suspend_def_triggertask_select(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (!tinfo->i.tp_select.st)
        printf("suspend_trigger_task/tproc_select: st=0\n");
    else
        sched_select_task_suspend(tinfo->i.tp_select.st);
}
static void suspend_def_triggertask_poll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (!tinfo->i.tp_poll.t)
        printf("suspend_trigger_task/tproc_poll: t=0\n");
    else
        sched_adjust_prio_t(tinfo->i.tp_poll.t, 0, 0);
}
static void suspend_def_triggertask_signal(struct triggerinfo* trinfo)
{
}
static void suspend_def_triggertask_timer(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (!tinfo->i.tp_timer.t) {
        printf("suspend_trigger_task/tproc_timer: t=0\n");
    } else {
        sched_remove_task(tinfo->i.tp_timer.t);
        tinfo->i.tp_timer.t=0;
    }
}
/*---------------------------------------------------------------------------*/
static void reactivate_def_triggertask_select(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (!tinfo->i.tp_select.st)
        printf("reactivate_trigger_task/tproc_select: st=0\n");
    else
        sched_select_task_reactivate(tinfo->i.tp_select.st);
}
static void reactivate_def_triggertask_poll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (!tinfo->i.tp_poll.t)
        printf("reactivate_trigger_task/tproc_poll: t=0\n");
    else
        sched_adjust_prio_t(tinfo->i.tp_poll.t, READOUT_PRIOR, 0);
}
static void reactivate_def_triggertask_signal(struct triggerinfo* trinfo)
{
}
static void reactivate_def_triggertask_timer(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    union callbackdata calldata;
    calldata.p=0;

    if (!tinfo->i.tp_timer.t) {
        tinfo->i.tp_timer.t=sched_exec_periodic(trigger_task,
                calldata, tinfo->i.tp_timer.time, "trigger");
    }
    if (!tinfo->i.tp_timer.t) {
        printf("reactivate_readout: kann periodic task nicht installieren\n");
/*
 * XXX should fatal_readout_error be called for the real trigger only?
 *     But a nonworking LAM may also be fatal.
 */
        fatal_readout_error();
    }
}
/*---------------------------------------------------------------------------*/
static void remove_def_triggertask_select(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (tinfo->i.tp_select.st)
        sched_delete_select_task(tinfo->i.tp_select.st);
    tinfo->i.tp_select.st=0;
}
static void remove_def_triggertask_poll(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (tinfo->i.tp_poll.t)
        sched_remove_task(tinfo->i.tp_poll.t);
    tinfo->i.tp_poll.t=0;
}
static void remove_def_triggertask_signal(struct triggerinfo* trinfo)
{
}
static void remove_def_triggertask_timer(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (tinfo->i.tp_timer.t)
        sched_remove_task(tinfo->i.tp_timer.t);
    tinfo->i.tp_timer.t=0;
}
/*****************************************************************************/
errcode
init_trigger(struct triggerinfo* trinfo, int proc, ems_u32* args)
{
    struct trigprocinfo* tinfo;
    plerrcode res=OK;

    T(init_trigger)

    /* trigger is not part of an IS but can use modules */
    memberlist=0;

    if (proc>=NrOfTrigs)
        return Err_NoSuchTrig;

    trinfo->state=trigger_idle;
    tinfo=calloc(1, sizeof(struct trigprocinfo));
    if (!tinfo) {
        printf("init_trigger:calloc: %s\n", strerror(errno));
        return Err_NoMem;
    }
    trinfo->tinfo=tinfo;
    tinfo->proctype=tproc_invalid;
    tinfo->magic=0;

    if ((res=Trig[proc].init_trig(args, trinfo))) {
        *outptr++=res;
        free(tinfo);
        return Err_TrigProc;
    }

    /* 
     * make some informational output and check whether all necessary
     * fields in trigprocinf are set
     */
    printf("init_trigger: trigproc \"%s\" installed as type \"%s\"\n",
            Trig[proc].name_trig,
             trigtypenames[tinfo->proctype]);
    switch (tinfo->proctype) {
    case tproc_other:
        /* useless? */
        break;
    case tproc_select:
        printf("path=%d seltype=%d\n",
                tinfo->i.tp_select.path,
                tinfo->i.tp_select.seltype);
        if (!tinfo->get_trigger) {
            printf("init_trigger: get_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->reset_trigger) {
            printf("init_trigger: reset_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->done_trigger) {
            printf("init_trigger: done_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->insert_triggertask)
            tinfo->insert_triggertask=insert_def_triggertask_select;
        if (!tinfo->suspend_triggertask)
            tinfo->suspend_triggertask=suspend_def_triggertask_select;
        if (!tinfo->reactivate_triggertask)
            tinfo->reactivate_triggertask=reactivate_def_triggertask_select;
        if (!tinfo->remove_triggertask)
            tinfo->remove_triggertask=remove_def_triggertask_select;
        break;
    case tproc_poll:
        if (!tinfo->get_trigger) {
            printf("init_trigger: get_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->reset_trigger) {
            printf("init_trigger: reset_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->done_trigger) {
            printf("init_trigger: done_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->insert_triggertask)
            tinfo->insert_triggertask=insert_def_triggertask_poll;
        if (!tinfo->suspend_triggertask)
            tinfo->suspend_triggertask=suspend_def_triggertask_poll;
        if (!tinfo->reactivate_triggertask)
            tinfo->reactivate_triggertask=reactivate_def_triggertask_poll;
        if (!tinfo->remove_triggertask)
            tinfo->remove_triggertask=remove_def_triggertask_poll;
        break;
    case tproc_signal:
        printf("not yet implemented\n");
        if (!tinfo->insert_triggertask)
            tinfo->insert_triggertask=insert_def_triggertask_signal;
        if (!tinfo->suspend_triggertask)
            tinfo->suspend_triggertask=suspend_def_triggertask_signal;
        if (!tinfo->reactivate_triggertask)
            tinfo->reactivate_triggertask=reactivate_def_triggertask_signal;
        if (!tinfo->remove_triggertask)
            tinfo->remove_triggertask=remove_def_triggertask_signal;
        res=Err_Other;
        break;
    case tproc_timer:
        printf("time=%d\n", tinfo->i.tp_timer.time);
        if (!tinfo->get_trigger) {
            printf("init_trigger: get_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->reset_trigger) {
            printf("init_trigger: reset_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->done_trigger) {
            printf("init_trigger: done_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->insert_triggertask)
            tinfo->insert_triggertask=insert_def_triggertask_timer;
        if (!tinfo->suspend_triggertask)
            tinfo->suspend_triggertask=suspend_def_triggertask_timer;
        if (!tinfo->reactivate_triggertask)
            tinfo->reactivate_triggertask=reactivate_def_triggertask_timer;
        if (!tinfo->remove_triggertask)
            tinfo->remove_triggertask=remove_def_triggertask_timer;
        break;
    case tproc_callback:
        if (!tinfo->insert_triggertask) {
            printf("init_trigger: insert_triggertask not set\n");
            res=Err_Program;
        }
        if (!tinfo->suspend_triggertask) {
            printf("init_trigger: suspend_triggertask not set\n");
            res=Err_Program;
        }
        if (!tinfo->reactivate_triggertask) {
            printf("init_trigger: reactivate_triggertask not set\n");
            res=Err_Program;
        }
        if (!tinfo->remove_triggertask) {
            printf("init_trigger: remove_triggertask not set\n");
            res=Err_Program;
        }
        if (tinfo->get_trigger) {
            printf("init_trigger: get_trigger set (but it should not!)\n");
            res=Err_Program;
        }
        if (!tinfo->reset_trigger) {
            printf("init_trigger: reset_trigger not set\n");
            res=Err_Program;
        }
        if (!tinfo->done_trigger) {
            printf("init_trigger: done_trigger not set\n");
            res=Err_Program;
        }
        break;
    case tproc_invalid:
    default:
        printf("init_trigger: invalid procedure type.\n");
        res=Err_Program;
        break;
    }

    return res;
}
/*****************************************************************************/
int
get_trigger(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    return tinfo->get_trigger(trinfo);
}
/*****************************************************************************/
void
reset_trigger(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    tinfo->reset_trigger(trinfo);
}
/*****************************************************************************/
errcode
insert_trigger_task(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    errcode res=Err_Program;

    printf("insert_trigger_task\n");
    if (!tinfo) {
        printf("insert_trigger: was not initialized");
        return Err_Program;
    }
    if (tinfo->insert_triggertask) {
        res=tinfo->insert_triggertask(trinfo);
    } else {
        printf("insert_trigger: no insertion procedure defined\n");
    }
    if (res==OK)
        trinfo->state|=trigger_inserted|trigger_active;
    return res;
}
/*****************************************************************************/
void
remove_trigger_task(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;

    if (!(trinfo->state&trigger_inserted)) {
        printf("remove_trigger: task was not inserted\n");
        return;
    }
    trinfo->state=trigger_idle;
    if (tinfo->remove_triggertask) {
        tinfo->remove_triggertask(trinfo);
    } else {
        printf("remove_trigger: no remove procedure defined\n");
    }
}
/*****************************************************************************/
void
suspend_trigger_task(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;

    if (!(trinfo->state&trigger_active)) {
        printf("remove_trigger: task is not active\n");
        return;
    }
    trinfo->state&=~trigger_active;
    if (tinfo->suspend_triggertask) {
        tinfo->suspend_triggertask(trinfo);
    } else {
        printf("suspend_trigger: no suspend procedure defined\n");
    }
}
/*****************************************************************************/
void
reactivate_trigger_task(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;

    if (!(trinfo->state&trigger_inserted) || (trinfo->state&trigger_active)) {
        printf("reactivate_trigger: task in wrong state: %d\n", trinfo->state);
        return;
    }
    if (tinfo->reactivate_triggertask) {
        tinfo->reactivate_triggertask(trinfo);
        trinfo->state|=trigger_active;
    } else {
        printf("reactivate_trigger: no reactivation procedure defined\n");
    }
}
/*****************************************************************************/
errcode
done_trigger(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    plerrcode pres;
    errcode res=OK;

    if (!tinfo) {
        printf("done_trigger: was not initialized");
        return Err_Program;
    }

    if (trinfo->state&trigger_inserted)
        remove_trigger_task(trinfo);
    if ((pres=tinfo->done_trigger(trinfo))) {
        *outptr++=res;
        res=Err_TrigProc;
    }
    free(tinfo);
    trinfo->tinfo=0;
    trinfo->state=trigger_idle;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
