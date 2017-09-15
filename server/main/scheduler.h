/*
 * main/scheduler.h
 * $ZEL: scheduler.h,v 1.6 2007/05/21 20:02:50 wuestner Exp $
 */

#ifndef _scheduler_h_
#define _scheduler_h_

#include "timers.h"
#include "callbacks.h"

enum sched_type {
    sched_idle,
    sched_polling,
    sched_blocking,
    sched_timer,
    sched_path
};

struct plistentry {
    void (*proc)(union callbackdata);
    union callbackdata arg;
    struct plistentry *next;
};

struct taskdescr {
    enum sched_type typ;
    void (*proc)(union callbackdata);
    union callbackdata arg;
    char *name;
    union {
        struct {
            int prio1, prio2, p2;
            void (*blproc)(union callbackdata);
            void (*pproc)(union callbackdata);
            struct taskdescr *prev, *next;
            int insert_pending, remove_pending;
        } pollparms;
        struct {
            struct timerentry* timer;
        } timerparms;
    } schedparms;
    struct plistentry pl;
};

#ifndef OSK
enum select_types {select_read=1, select_write=2, select_except=4};
/*typedef void (*sel_pp)(int, select_types, callbackdata);*/
/* void(*)(int, enum select_types, union callbackdata) */
struct seltaskdescr {
    struct seltaskdescr* next;
    struct seltaskdescr* prev;
    void (*proc)(int, enum select_types, union callbackdata);
    union callbackdata arg;
    char *name;
    int fds;
    enum select_types types;
#ifdef SELECT_STATIST
    struct {
        struct timeval tv;
        int active;
        double enabled;
        double disabled;
    } statist;
#endif  
};

struct seltaskdescr* get_first_select_task(void);
struct seltaskdescr* get_next_select_task(struct seltaskdescr *h);
void sched_print_select_tasks(void);
struct seltaskdescr *sched_insert_select_task(
        void(*)(int, enum select_types, union callbackdata),
        union callbackdata, char*, int, enum select_types
#ifdef SELECT_STATIST
    ,int active
#endif
    );
void sched_delete_select_task(struct seltaskdescr*);
void sched_select_task_set(struct seltaskdescr*, enum select_types
#ifdef SELECT_STATIST
    ,int active
#endif
    );
enum select_types sched_select_task_get(struct seltaskdescr*);
#ifdef SELECT_STATIST
void sched_select_task_get_statist(struct seltaskdescr* descr, double* enabled,
    double* disabled, int clear);
void sched_select_get_statist(double* runtime, double* idle, int clear);
#endif
void sched_select_task_suspend(struct seltaskdescr*);
void sched_select_task_reactivate(struct seltaskdescr*);
#ifdef SELECT_STATIST
struct t_sched_select_statist {
    struct timeval tv;
    double idle;
};
extern struct t_sched_select_statist sched_select_statist;
#endif  

#endif /* OSK */

#define p_prio1 schedparms.pollparms.prio1
#define p_prio2 schedparms.pollparms.prio2
#define p_p2 schedparms.pollparms.p2
#define p_blproc schedparms.pollparms.blproc
#define p_pproc schedparms.pollparms.pproc
#define p_prev schedparms.pollparms.prev
#define p_next schedparms.pollparms.next
#define p_insert_pending schedparms.pollparms.insert_pending
#define p_remove_pending schedparms.pollparms.remove_pending
#define t_timer schedparms.timerparms.timer

struct taskdescr* sched_insert_poll_task(void(*)(union callbackdata),
        union callbackdata, int, int, char*);
struct taskdescr* sched_insert_block_task(void(*)(union callbackdata),
        void(*)(union callbackdata), union callbackdata, int, int, char*);
void sched_remove_task(struct taskdescr*);
/*void sched_exec_once(struct plistentry*);*/
void sched_exec_once(union callbackdata);
/*taskdescr *exec_later(void(*)(union callbackdata), int, char*);*/
struct taskdescr *sched_exec_periodic(void(*)(union callbackdata),
        union callbackdata, int, char*);

int sched_adjust_prio(char* name, int p1, int p2);
int sched_adjust_prio_t(struct taskdescr* t, int p1, int p2);

int init_sched(void);
void done_sched(void);
void sched_mainloop(void);
void sched_print_tasks(void);

extern volatile int breakflag;

#endif
