#include <signal.h>

#include "signals.h"

#define SIG_OFFSET 1024
#define SIG_ANZAHL 256

static signalproc handler[SIG_ANZAHL];
static SigHdlArg clientdata[SIG_ANZAHL];
static char *names[SIG_ANZAHL];
static char flag[SIG_ANZAHL];

static signalproc0 breakproc;

/*****************************************************************************/

static void ignore(){}

/*****************************************************************************/

int install_signalhandler(signalproc hand, SigHdlArg arg, char* name)
{
  int i;
  for(i=0; i<SIG_ANZAHL; i++){
    if(!flag[i]){
      handler[i]=hand;
      clientdata[i]=arg;
      names[i]=name;
      flag[i]=1;
      return(i+SIG_OFFSET);
    }
  }
  return(-1);
}

/*****************************************************************************/

int remove_signalhandler(sig)
int sig;
{
  sig-=SIG_OFFSET;
  if ((sig<0)||(sig>=SIG_ANZAHL)||(!flag[sig])) return(-1);
  handler[sig]=ignore;
  flag[sig]=0;
  return(0);
}

/*****************************************************************************/

static void hndlr(sig)
int sig;
{
  register int index;
  index=sig-SIG_OFFSET;
  if((unsigned)index<SIG_ANZAHL){
    handler[index](clientdata[index].val, sig); /* XXXX */
    return;
  }
  printf("%s Signal %d erhalten, tschuess!\n",
	 ((sig==SIGQUIT)||(sig==SIGINT))?"toedliches":"unerwartetes", sig);
  if(sig!=SIGQUIT){
    if(breakproc){
      printf("Gebe Ressourcen frei bzw. versuche dies.\n");
      (*breakproc)();
    }
  }
  exit(sig);
}

/*****************************************************************************/

int init_signalhandler(){
  int i;
  for(i=0; i<SIG_ANZAHL; i++){
    handler[i]=ignore;
    flag[i]=0;
  }
  breakproc=(signalproc0)0;
  intercept(hndlr);
  return(0);
}

void done_signalhandler(){
  intercept(0);
}

void disable_breaks(){
  sigmask(1);
}

void enable_breaks(){
  sigmask(-1);
}

int install_shutdownhandler(proc)
signalproc0 proc;
{
  if(breakproc)return(-1);
  breakproc=proc;
  return(0);
}
