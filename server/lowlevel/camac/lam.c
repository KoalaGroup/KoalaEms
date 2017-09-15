/*
 * lowlevel/camac/lam.c
 *
 * created before 22.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lam.c,v 1.9 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <debug.h>

#if defined(unix) || defined(__unix__)
#include <unistd.h>
#include <signal.h>
#endif

#include <errorcodes.h>
#include <rcs_ids.h>

#include "camac.h"

static int lammask,alm,timerflag;

#ifdef OSK
static int sig;
#else
static int almval;
#endif

RCS_REGISTER(cvsid, "lowlevel/camac")

/*****************************************************************************/

int get_lam()
{
int lamstat=(CAMACval(CAMACread(CAMACaddr(30,0,0)))|(timerflag<<31))&lammask;
timerflag=0;
return(lamstat);
}

/*****************************************************************************/

static void lamhndlr(s)
int s;
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

int get_lam_mask(idx)
int idx;
{
if (idx)
  return(1<<(idx-1));
else
  return(0x80000000);
}

/*****************************************************************************/

void enable_lam(int idx, int argnum, int* args)
{
if ((idx==0) && (argnum>0))
#ifdef OSK
alm=alm_cycle(sig, 100*args[0]);
#else
{
almval=args[0];
alarm(almval);
alm=42;
}
#endif
lammask|=get_lam_mask(idx);
}

/*****************************************************************************/

void disable_lam(idx)
int idx;
{
if (idx==0)
#ifdef OSK
alm_delete(alm);
#else
alarm(0);
#endif
lammask&=~get_lam_mask(idx);
}

/*****************************************************************************/
/*****************************************************************************/
