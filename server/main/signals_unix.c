/*
 * main/signals_unix.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: signals_unix.c,v 1.15 2014/09/10 15:28:59 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rcs_ids.h>

#include "signals.h"
#include "childhndlr.h"

RCS_REGISTER(cvsid, "main")

#ifdef Lynx
static int signumbers[]={
  SIGUDEF24,
  SIGUDEF25,
  SIGUDEF28,
  SIGUDEF29,
  SIGUDEF30,
  SIGUDEF31,
};
#define SIG_ANZAHL 6
#else
#ifdef GO32
#define signumbers ((int*)0)
#define SIG_ANZAHL 0
#else

#ifdef __linux__
#define USE_RT_SIGNALS
#else
#undef USE_RT_SIGNALS
#endif

#ifdef USE_RT_SIGNALS
/* in LINUX SIGRTMAX and SIGRTMIN are not constants */
/* __SIGRTMAX and __SIGRTMIN are hard kernel limits but should not be used */
#define SIG_ANZAHL (__SIGRTMAX-__SIGRTMIN)
#else
static int signumbers[]={
  SIGUSR1,
  SIGUSR2
};
#define SIG_ANZAHL 2
#endif

#endif
#endif

static signalproc handler[SIG_ANZAHL];
static union SigHdlArg clientdata[SIG_ANZAHL];
static char *names[SIG_ANZAHL];
static sigset_t ourmask;

static int timerhandler_installed;
static signalproc0 breakproc;

/*****************************************************************************/
static int
signumber(int idx)
{
#ifdef USE_RT_SIGNALS
    return idx+SIGRTMIN;
#else
    return signumbers[idx];
#endif
}
/*****************************************************************************/
static int
sigindex(int sig)
{
#ifdef USE_RT_SIGNALS
    if ((sig<SIGRTMIN)||(sig>=SIGRTMAX))
        return -1;
    return sig-SIGRTMIN;
#else
    int i;
    for (i=0; i<SIG_ANZAHL; i++) {
        if (signumbers[i]==sig)
            return i;
    }
    return -1;
#endif
}
/*****************************************************************************/
static int
find_free_index(void)
{
#ifdef USE_RT_SIGNALS
    int sig, idx;
    for (sig=SIGRTMIN; sig<SIGRTMAX; sig++) {
        idx=sigindex(sig);
        if (!handler[idx])
            return idx;
    }
    return -1;
#else
    int idx;
    for (idx=0; idx<SIG_ANZAHL; idx++){
    if (!handler[idx])
        return idx;
    }
    return -1;
#endif
}
/*****************************************************************************/
static void hndlr(int sig)
{
    int idx;
    idx=sigindex(sig);
    if (idx>=0)
        handler[idx](clientdata[idx], sig);
}
/*****************************************************************************/
static int installsig(int sig, void(*h)(int))
{
  struct sigaction act;
  act.sa_handler=h;
  act.sa_mask=ourmask;
  act.sa_flags=0;
  if (sigaction(sig, &act, 0)<0) {
    printf("sigaction(%d): %s\n", sig, strerror(errno));
    return -1;
  }
  return(0);
}
/*****************************************************************************/
static int removesig(int sig)
{
  struct sigaction act;
  sigset_t s;

  act.sa_handler=SIG_DFL;
  sigemptyset(&s);
  act.sa_mask=s;
  act.sa_flags=0;
  if(sigaction(sig,&act,0)<0)return(-1);
  return(0);
}
/*****************************************************************************/
int install_signalhandler(signalproc hand, union SigHdlArg arg, char* name)
{
    int i, sig;
    i=find_free_index();
    if (i<0) {
        printf("no more signals\n");
        return -1;
    }
    sig=signumber(i);
    if (installsig(sig, hndlr)) {
	printf("install sighandler %d failed\n", i);
	return -1;
    }
    handler[i]=hand;
    clientdata[i]=arg;
    names[i]=strdup(name);
printf("signalhandler with sig %d installed\n", sig);
    return sig;
}
/*****************************************************************************/
int install_signalhandler_sig(signalproc hand, int sig, union SigHdlArg arg,
        char* name)
{
    int i;

    i=sigindex(sig);
    if (i<0 || handler[i]) {
        printf("signal %d for %s not available\n", sig, name);
        return -1;
    }

    if (installsig(sig, hndlr)) {
	printf("install sighandler %d failed\n", i);
	return -1;
    }
    handler[i]=hand;
    clientdata[i]=arg;
    names[i]=strdup(name);
printf("signalhandler for sig %d installed\n", sig);
    return sig;
}
/*****************************************************************************/
static int removehandler(int idx)
{
  int sig=signumber(idx);
  if(removesig(sig)){
    printf("remove sighandler %d failed\n",idx);
    return(-1);
  }
  handler[idx]=(signalproc)0;
  free(names[idx]);
  names[idx]=0;
  return(0);
}
/*****************************************************************************/
int remove_signalhandler(int sig)
{
    int i;
    i=sigindex(sig);
    if (i<0) {
        printf("invalid signal %d\n",sig);
        return -1;
    }
    if (!handler[i])
        return -1;
    if (removehandler(i))
        return -1;
printf("signalhandler for sig %d removed\n", sig);
    return 0;
}
/*****************************************************************************/
static void ende(int sig){
  if(breakproc)(*breakproc)();
#ifndef GO32
  exit(0);
#endif
}
/*****************************************************************************/
int init_signalhandler()
{
  int i;
  sigemptyset(&ourmask);
  for(i=0;i<SIG_ANZAHL;i++){
    handler[i]=(signalproc)0;
    sigaddset(&ourmask,signumber(i));
  }
  timerhandler_installed=0;
  breakproc=(signalproc0)0;
#ifdef GO32
  onexit(ende);
#else
  sigaddset(&ourmask,SIGALRM);
  if(installsig(SIGINT,ende))return(-1);
  installsig(SIGPIPE,SIG_IGN);
  if (installsig(SIGCHLD, child_dispatcher))
      printf("cannot install handler for SIGCHLD: %s\n", strerror(errno));
#endif
  return(0);
}
/*****************************************************************************/
void done_signalhandler(){
  int i;
  for(i=0;i<SIG_ANZAHL;i++){
    if(handler[i])removehandler(i);
  }
  if(timerhandler_installed)removesig(SIGALRM);
#ifdef GO32
  onexit(0);
#else
  removesig(SIGINT);
#endif
}
/*****************************************************************************/
void disable_breaks(sigresc* m)
{
  sigprocmask(SIG_BLOCK,&ourmask,m);
}
/*****************************************************************************/
void enable_breaks(sigresc* m)
{
  sigprocmask(SIG_SETMASK,m,0);
}
/*****************************************************************************/
int install_timerhandler(void(*proc)(int))
{
  if(timerhandler_installed)return(-1);
  if(installsig(SIGALRM,proc))return(-1);
  timerhandler_installed=1;
  return(0);
}
/*****************************************************************************/
int install_shutdownhandler(signalproc0 proc)
{
  if(breakproc)return(-1);
  breakproc=proc;
  return(0);
}
/*****************************************************************************/
void print_signals(){
  int i;

  printf("Signale:\n");

  for(i=0;i<SIG_ANZAHL;i++){
    if(handler[i])
      printf(" %s: %d\n",names[i],signumber(i));
  }
}
/*****************************************************************************/
/*****************************************************************************/
