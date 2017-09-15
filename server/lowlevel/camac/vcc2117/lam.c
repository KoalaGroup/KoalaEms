#if defined(unix) || defined(__unix__) || defined(Lynx)
#include <unistd.h>
#include <signal.h>
#endif

#include <errorcodes.h>

#include "../camac.h"

static int lammask,alm,timerflag;

#ifdef OSK
static int sig;
#else
static int almval;
#endif

/*****************************************************************************/

int get_lam()
{
  int lamstat=(CAMAClams()|(timerflag<<31))&lammask;
  timerflag=0;
  return(lamstat);
}

/*****************************************************************************/

static void lamhndlr(int s)
{
timerflag=1;
#ifndef OSK
alarm(almval);
#endif
}

/*****************************************************************************/

errcode init_lam()
{
lammask=0;
timerflag=0;
alm= -1;
#ifdef OSK
sig=install_signalhandler(lamhndlr);
#else
signal(SIGALRM,lamhndlr);
#endif
return(OK);
}

/*****************************************************************************/

errcode done_lam()
{
if (alm!=-1)
#ifdef OSK
  alm_delete(alm);
#else
  alarm(0);
#endif
#ifdef OSK
remove_signalhandler(sig);
#else
signal(SIGALRM,SIG_DFL);
#endif
return(OK);
}

/*****************************************************************************/

int get_lam_mask(int idx)
{
if (idx)
  return(1<<(idx-1));
else
  return(0x80000000);
}

/*****************************************************************************/

void enable_lam(int idx, int arg)
{
if ((!idx)&&arg)
#ifdef OSK
alm=alm_cycle(sig,100*arg);
#else
{
almval=arg;
alarm(almval);
alm=42;
}
#endif
lammask|=get_lam_mask(idx);
}

/*****************************************************************************/

void disable_lam(int idx)
{
if (!idx)
#ifdef OSK
alm_delete(alm);
#else
alarm(0);
#endif
lammask&=~get_lam_mask(idx);
}

/*****************************************************************************/
/*****************************************************************************/

