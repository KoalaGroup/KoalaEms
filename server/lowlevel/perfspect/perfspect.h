/*
 * lowlevel/perfspect/perfspect.h
 * 
 * $ZEL: perfspect.h,v 1.7 2007/04/19 22:17:29 wuestner Exp $
 */

#ifndef _perfspect_h_
#define _perfspect_h_

#include <sconf.h>

#ifdef PERFSPECT

#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"

struct perf_dev;

#if 0
#ifdef PERFSPECT_ZELSYNC
#include "zelsync/zelsync_perfspect.h"
#endif
#endif

enum perftypes {perf_none, perf_pcisync};

struct perf_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    unsigned int (*perf_time)(struct perf_dev*);
    unsigned int (*perf_unit)(struct perf_dev*);
    void         (*perf_hw_eventstart)(struct perf_dev*);
#ifdef DELAYED_READ
    /* this function should be hardware (controller) dependend */
    int (*delay_read)(struct perf_dev*, off_t src, void* dst, size_t size);
#endif

/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    enum perftypes perftype;
    char* pathname;
    void* info;
#if 0
    union {
#ifdef PERFSPECT_ZELSYNC
        struct perfspect_pcisync_info pcisync;
#endif
    } info; 
#endif
};

enum perfspect_state {
    perfspect_state_invalid,
    perfspect_state_setup,
    perfspect_state_ready,
    perfspect_state_inactive,
    perfspect_state_active,
};

enum perfspect_code {
    perfcode_trig_is=1,
    perfcode_proc,
};

errcode perfspect_low_init(char* arg);
errcode perfspect_low_done(void);
int perfspect_low_printuse(FILE* outfilepath);

void perfspect_reset(void);
plerrcode perfspect_realloc_arrays(void);
void perfspect_set_state(enum perfspect_state state);
void perfspect_set_trigg_is(int unsigned trig, unsigned int is);
void perfspect_set_proc(int idx, int proc);

void perfspect_alloc_isdata(int numproc);
void perfspect_alloc_procdata(int proc_idx, int proc_id, const char* name,
        int num, char** names);

/* hardware dependend part (i.e. zelsync/zelsync_perfspect.c) */
/*errcode perfspect_init(char* path);
errcode perfspect_done(void);*/
void perfspect_eventstart(void);
void perfspect_hw_eventstart(void);
void perfspect_start_proc(int proc_idx);
void perfspect_end_proc(void);
void perfspect_record_time(char* caller);
unsigned int perfspect_time(void);
unsigned int perfspect_unit(void); /* only for user interface */

/* user interface (for procs/perfspect/procperfspect.c) */
plerrcode perfspect_get_spect(unsigned int trig, unsigned int is,
        unsigned int proc, int ident, ems_u32**);
plerrcode perfspect_get_info(ems_u32*, ems_u32** outptr);
plerrcode perfspect_infotree(ems_u32** outptr);
void perfspect_get_setup(int* scale, int* size);
plerrcode perfspect_set_setup(int scale, int size);
int perfspect_readout_stack(ems_u32* outptr);

plerrcode perfspect_set_dev(int idx);
struct perf_dev* perfspect_get_dev(void);

#else /* PERFSPECT */

#define perfspect_reset()                do {} while (0)
#define perfspect_set_state(state)       do {} while (0)
#define perfspect_set_trigg_is(trig, is) do {} while (0)
#define perfspect_set_proc(idx, iproc)   do {} while (0)

#define perfspect_alloc_isdata(numproc)  do {} while (0)
#define perfspect_alloc_procdata(proc_idx, proc_id, name, num, names) do {} while (0)

#define perfspect_eventstart()           do {} while (0)
#define perfspect_start_proc(proc_idx)   do {} while (0)
#define perfspect_end_proc()             do {} while (0)
#define perfspect_record_time(caller)    do {} while (0)

#endif /* PERFSPECT */

#if 0

errcode perfspect_init(char* path);
errcode perfspect_done(void);

/*
 * int perfspect_add_current(int id);
 * void perfspect_add(int id, int tics);
 * int propagate_perfinfo(int path);
 */

void delete_perfspect_data(void);
void clear_perfspect_data(void);

int perfspect_allocate_is(unsigned int trig, unsigned int is,
        unsigned int numproc);
int perfspect_allocate_procdata(unsigned int num);
void perfspect_set_procid(int id);

void perfspect_set_is_ptr(unsigned int trig, unsigned int is);
void perfspect_set_proc_ptr(unsigned int proc);

void perfspect_proc_start(void);
void perfspect_proc_end(void);
void perfspect_proc_extra(unsigned int idx);
void perfspect_event_begin(ems_u32 eventcnt);
#define perfspect_event_end(eventcnt)

void* perfspect_spectptr(unsigned int trig, unsigned int is, unsigned int proc,
        int ident);
int perfspect_dump_spect(void* vspect, unsigned int* out);
int perfspect_infotree(unsigned int* out);
int perfspect_set(int trig, int is, int proc, int ident, int shift);

/* defined in subdirectories */

int perfspect_current(void);

#endif /* 0 */

#endif
