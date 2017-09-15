/*
 * main/signals.h
 * $ZEL: signals.h,v 1.8 2008/06/26 14:40:53 wuestner Exp $
 */

#ifndef _signals_h_
#define _signals_h_

#include <sconf.h>
#include <debug.h>

#include <errno.h>
#ifdef OSK
typedef int sigresc; /* dummy */
#else
#include <signal.h>
typedef sigset_t sigresc;
#endif

union SigHdlArg{
    int val;
    void *ptr;
};

typedef void (*signalproc)(union SigHdlArg, int);
typedef void (*signalproc0)(void);

int install_signalhandler(signalproc hand, union SigHdlArg arg, char* name);
int install_signalhandler_sig(signalproc hand, int sig, union SigHdlArg arg,
        char* name);
int remove_signalhandler(int sig);
#ifndef OSK
int install_timerhandler(void(*proc)(int));
#endif
int install_shutdownhandler(signalproc0 proc);
void disable_breaks(sigresc* m);
void enable_breaks(sigresc* m);

int init_signalhandler(void);
void done_signalhandler(void);
void print_signals(void);

#endif
