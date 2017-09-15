#include "signals.h"
#include "timers.h"

int install_timeout(timeout,proc,arg,name,resc)
int timeout;
timeoutproc proc;
TimeHdlArg arg;
char *name;
timeoutresc *resc;
{
  resc->sig=install_signalhandler(proc, arg, name);
  resc->alm=alm_set(resc->sig,timeout*100/TIMER_RES);
  if(resc->alm==-1){
    remove_signalhandler(resc->sig);
    return(-1);
  }
  return(0);
}

int install_periodic(int zyklus, timeoutproc proc, TimeHdlArg arg, char* name,
    timeoutresc* resc)
{
  resc->sig=install_signalhandler(proc, arg, name);
  resc->alm=alm_cycle(resc->sig,zyklus*100/TIMER_RES);
  if(resc->alm==-1){
    remove_signalhandler(resc->sig);
    return(-1);
  }
  return(0);
}

int remove_timer(resc)
timeoutresc *resc;
{
  alm_delete(resc->alm);
  remove_signalhandler(resc->sig);
  return(0);
}

int init_timers(){
  return(0);
}

void done_timers(){}
