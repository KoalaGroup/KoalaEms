/*
 * main/timers.h
 * $ZEL: timers.h,v 1.5 2007/05/21 20:02:50 wuestner Exp $
 */

#ifndef _timers_h_
#define _timers_h_

#include "callbacks.h"

#ifdef OSK

typedef struct timeoutresc {
  int sig;
  int alm;
} timeoutresc;

#else
#include <sys/time.h>

struct timerentry {
    struct timeval t;
/*  timeoutproc proc;
    TimeHdlArg arg;*/
    void (*proc)(union callbackdata);
    union callbackdata arg;
    int periodic;
    char *name;
    struct timerentry *next;
};

typedef struct timerentry *timeoutresc;
#endif

/*
 * int install_timeout(int,timeoutproc,TimeHdlArg,char*,timeoutresc*);
 * int install_periodic(int,timeoutproc,TimeHdlArg,char*,timeoutresc*);
 */
int install_timeout(int, void(*)(union callbackdata), union callbackdata,
        char*, struct timerentry**);
int install_periodic(int, void(*)(union callbackdata), union callbackdata,
        char*, struct timerentry**);
int remove_timer(struct timerentry**);

int init_timers(void);
void done_timers(void);
void print_timers(void);

#define TIMER_RES 100 /* pro sec */

#endif
