/*
 * $ZEL: trigger.h,v 1.8 2009/07/27 22:14:26 wuestner Exp $
 */

#ifndef _trigger_h_
#define _trigger_h_

#include <errorcodes.h>
#include <emsctypes.h>

/*
 * This is the exported interface of the trigger procedure mechanism.
 * Trigger procedures may be used for the real trigger and to trigger
 * generalized LAM procedures. Generalized LAM procedures are triggered
 * by an external (i.e. hardware) or internal (i.e. timer) event. Arbitrary
 * ems procedures can then be executed.
 * All trigger procedures use two data structures: trigprocinfo and triggerinfo.
 * trigprocinfo is used internally only and must not be exported the 
 * trigger tree.
 * triggerinfo is exported and contains information about the real trigger.
 * It is more or less useless for LAMs.
 * calls related to the real trigger have to use 'trigger' as triggerinfo.
 * Trigger procedures must not have static variables. All necessary
 * private information has to be contained in triggerinfo.tinfo.private 
 */

/*
 * triggerstate is used as a bitmask:
 *     triggerstate==0: not initialized or initialized but trigger procedure
 *         not inserted
 *     triggerstate==trigger_inserted: trigger procedure inserted but
 *         not running
 *     triggerstate==trigger_inserted|trigger_active: trigger procedure
 *         running
 *     triggerstate==trigger_active: impossible
 *     note: insert_trigger_task inserts and starts the trigger procedure
 *         Is it possible to insert it in a disabled state?
 */
enum triggerstate {trigger_idle=0, trigger_inserted=1, trigger_active=2};

struct triggerinfo {
    ems_u32 eventcnt;
    ems_u32 trigger;
    struct timespec time;
    int time_valid;
    void (*cb_proc)(void*);
    void *cb_data;
    enum triggerstate state;
    void* tinfo; /* pointer to struct trigprocinfo and other private data */
};

/*
 * defined in trigprocs.c.m4, used in objects/pi/.../readout.c
 */
extern struct triggerinfo trigger;

/*
 * returns (in list) the names and indices of all trigger procedures
 */
errcode gettrigproclist(ems_u32* list, unsigned int);

/*
 * initializes a trigger procedure
 */
errcode init_trigger(struct triggerinfo*, int proc, ems_u32* args);
errcode done_trigger(struct triggerinfo*);

/*
 * checks whether a trigger has occured and return the trigger pattern
 * or 0 if no trigger is available (used in polling mode or after select)
 */
int     get_trigger(struct triggerinfo*);

/*
 * reenables the trigger after sucessfull readout or LAM
 */
void    reset_trigger(struct triggerinfo*);

/*
 * removes the trigger task temorarily from the scheduler
 */
void    remove_trigger_task(struct triggerinfo*);
/*
 * disables he trigger task temorarily (i.e. if the output stream is congested
 * if suspend_... is not available remove_... can be used instead
 */
void    suspend_trigger_task(struct triggerinfo*);

/*
 * inserts the trigger task into the scheduler (initial start or
 * restart after remove_trigger_task)
 */
errcode insert_trigger_task(struct triggerinfo*);
/*
 * reactivate the trigger task after suspend_...
 */
void    reactivate_trigger_task(struct triggerinfo*);

int trigger_init(void);
int trigger_done(void);

#endif
