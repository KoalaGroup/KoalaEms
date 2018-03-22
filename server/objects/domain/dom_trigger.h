/*
 * objects/domain/dom_trigger.h
 * created before 02.02.93
 * $ZEL: dom_trigger.h,v 1.8 2017/10/20 23:21:31 wuestner Exp $
 */

#ifndef _dom_trigger_h_
#define _dom_trigger_h_

#include <errorcodes.h>
#include <emsctypes.h>

struct triggerinfo; /* defined in trigger/trigger.h */

#if 0
/* informal copy from trigger/trigger.h */
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
#endif

struct triginfo {
    struct triginfo *next;
    struct triginfo *prev;
    struct triggerinfo *trinfo;
    unsigned int idx;
    unsigned int proc;
    ems_u32 param[1];
};

//extern triginfo trigdata;
extern struct triginfo *triginfo; /* defined in dom_trigger.c */

errcode dom_trigger_init(void);
errcode dom_trigger_done(void);

#endif

/*****************************************************************************/
/*****************************************************************************/

