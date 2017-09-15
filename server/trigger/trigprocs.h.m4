/*
 * trigger/trigprocs.h.m4
 * last changed: 20.10.1997
 */
/* $ZEL: trigprocs.h.m4,v 1.6 2007/10/15 14:22:35 wuestner Exp $ */

#ifndef _trigprocs_h_m4_
#define _trigprocs_h_m4_

#include <sconf.h>
#include <errorcodes.h>
#include <emsctypes.h>

/*
 * This is the internal interface of the trigger procedure mechanism.
 * Nothing must be exported from here outside the trigger tree.
 */

enum trigproctype {
    tproc_other, tproc_select, tproc_poll, tproc_signal, tproc_timer,
    tproc_callback,
    tproc_invalid /* must be always the last entry */
};

struct trigprocinfo {
    enum trigproctype proctype;
    void* magic; /* address of init procedure for sanity checks */
    errcode   (*insert_triggertask)(struct triggerinfo* info);
    void      (*suspend_triggertask)(struct triggerinfo* info);
    void      (*reactivate_triggertask)(struct triggerinfo* info);
    void      (*remove_triggertask)(struct triggerinfo* info);
    int       (*get_trigger)(struct triggerinfo* info);
    void      (*reset_trigger)(struct triggerinfo* info);
    plerrcode (*done_trigger)(struct triggerinfo* info);
    union {
        struct {
            int dummy;
        } tp_other;
        struct {
            struct seltaskdescr *st;
            int path;
            enum select_types seltype;
        } tp_select;
        struct {
            struct taskdescr *t;
        } tp_poll;
        struct {
            int dummy;
        } tp_signal;
        struct {
            struct taskdescr *t;
            int time;
        } tp_timer;
        struct {
            int dummy;
        } tp_callback;
    } i;
    void *private;
};

struct trigproc {
    plerrcode (*init_trig)(ems_u32*, struct triggerinfo*);
    char* name_trig;
    int* ver_trig;
};

extern struct trigproc Trig[];
extern int NrOfTrigs;

define(`trig',`
plerrcode init_trig_$1(ems_u32*, struct triggerinfo* info);
extern char name_trig_$1[];
extern int ver_trig_$1;')
include(procedures)

#endif
