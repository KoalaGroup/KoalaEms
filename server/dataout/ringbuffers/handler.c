/*
 * dataout/ringbuffers/handler.c
 * 
 * 24.08.94
 * $ZEL: handler.c,v 1.22 2011/04/06 20:30:22 wuestner Exp $
 */

#include <errno.h>
#include <signal.h>
#ifdef unix
#include <stdlib.h>
#endif

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "handlers.h"
#include "../dataout.h"
#include "../../main/signals.h"
#include "../../lowlevel/oscompat/oscompat.h"


RCS_REGISTER(cvsid, "dataout/ringbuffers")

/*****************************************************************************/

int outputerror_occured;

extern char **environ;
extern int os9forkc();

static struct handlerresc {
    char *name;
    int pid,evid,sig;
} hresc[MAX_DATAOUT];

static VOLATILE int *combuf;

extern ems_u32* outptr;

static int welchesdataout;
static char handlercomname[256];

/*****************************************************************************/

void handle_outputerror()
{
  printf("Tape Error\n");
  *outptr++=rtErr_OutDev;
  *outptr++=welchesdataout;
}

/*****************************************************************************/

static void free_handler_resources(h)
struct handlerresc *h;
{
  _ev_unlink(h->evid);
  _ev_delete(h->name);
  free(h->name);
  if(h->sig!=-1)remove_signalhandler(h->sig);
  h->pid=-1;
  h->evid= -1;
  h->sig= -1;
  h->name=(char*)0;
}

int startsockhndlr(i, host, port, name)
int i, host, port;
char *name;
{
  struct handlerresc *h;
  char hoststr[15], portstr[15];
#ifdef EVENT_BUFBEG
  char bufbegstr[15],buflenstr[15];
  char *argv[8];
#else
  char *argv[6];
#endif

  T(startsockhndlr)
  h= &hresc[i];
  h->evid=_ev_creat(HANDLER_IDLE,0,0,name);
  if(h->evid==-1)return(errno);
  h->name=(char*)malloc(strlen(name)+1);
  strcpy(h->name,name);
  argv[0]=SOCKETHANDLER;
  snprintf(hoststr, 15,"%d",host);
  argv[1]=hoststr;
  snprintf(portstr, 15,"%d",port);
  argv[2]=portstr;
  argv[3]=name;
  argv[4]=handlercomname;
#ifdef EVENT_BUFBEG
  if(!i){
    snprintf(bufbegstr, 15,"%x",EVENT_BUFBEG);
    argv[5]=bufbegstr;
    snprintf(buflenstr, 15,"%x",EVENT_BUFEND-EVENT_BUFBEG);
    argv[6]=buflenstr;
    argv[7]=0;
  }else
#endif
    argv[5]=0;
  /* Aufruf: insocket Hostnummer Portnummer name [Bufferstart Bufferlen] */
  h->pid=os9exec(os9forkc,argv[0],argv,environ,0,0,3);
  if(h->pid==-1){
    int help;
    help=errno;
    free_handler_resources(h);
    return(help);
  }
  return(0);
}

/*****************************************************************************/

int startlocalsockhndlr(i, sockname, name)
int i;
char *sockname, *name;
{
  struct handlerresc *h;
  char *argv[5];

  T(startlocalsockhndlr)
  h= &hresc[i];
  h->evid=_ev_creat(HANDLER_IDLE,0,0,name);
  if(h->evid==-1)return(errno);
  h->name=(char*)malloc(strlen(name)+1);
  strcpy(h->name,name);
  argv[0]=LOCALSOCKETHANDLER;
  argv[1]=sockname;
  argv[2]=name;
  argv[3]=handlercomname;
  argv[4]=0;
  /* Aufruf: inlocalsocket Pfad name */
  D(D_FORK, int i; printf("exec(");
      for (i=0; argv[i]!=0; i++)
          printf("%s%s", argv[i], (argv[i+1]!=0)?", ":")\n");)
  h->pid=os9exec(os9forkc,argv[0],argv,environ,0,0,3);
  if(h->pid==-1){
    int help;
    help=errno;
    free_handler_resources(h);
    return(help);
  }
  return(0);
}

/*****************************************************************************/

int startfilehndlr(i, path, name)
int i;
char *path, *name;
{
  struct handlerresc *h;
  char *argv[5];

  T(startfilehndlr)
  h= &hresc[i];
  h->evid=_ev_creat(HANDLER_IDLE,0,0,name);
  if(h->evid==-1)return(errno);
  h->name=(char*)malloc(strlen(name)+1);
  strcpy(h->name,name);
  argv[0]=FILEHANDLER;
  argv[1]=path;
  argv[2]=name;
  argv[3]=handlercomname;
  argv[4]=0;
  /* Aufruf: infile Pfad name */
  D(D_FORK, int i; printf("exec(");
      for (i=0; argv[i]!=0; i++)
          printf("%s%s", argv[i], (argv[i+1]!=0)?", ":")\n");)
  h->pid=os9exec(os9forkc,argv[0],argv,environ,0,0,3);
  if(h->pid==-1){
    int help;
    help=errno;
    free_handler_resources(h);
    return(help);
  }
  return(0);
}

/*****************************************************************************/

static void tapeerror(int sig, SigHdlArg arg)
{
  welchesdataout=arg.val;
  outputerror_occured=1;
}

/*****************************************************************************/

int starttapehndlr(int i, char* path, char* name)
{
  struct handlerresc *h;
  char *argv[6];
  char sigstr[15];
  SigHdlArg arg;

  T(starttapehndlr)
  h= &hresc[i];
  arg.val=i;
  h->sig=install_signalhandler(tapeerror,arg, name);
  if(h->sig==-1)return(E_MEMFUL);
  snprintf(sigstr, 15,"%d",h->sig);
  h->evid=_ev_creat(HANDLER_IDLE,0,0,name);
  if(h->evid==-1){
    remove_signalhandler(h->sig);
    return(errno);
  }
  h->name=(char*)malloc(strlen(name)+1);
  strcpy(h->name,name);
  argv[0]=TAPEHANDLER;
  argv[1]=path;
  argv[2]=name;
  argv[3]=sigstr;
  argv[4]=handlercomname;
  argv[5]=0;
  /* Aufruf: aufexa Pfad name Signalnummer */
  D(D_FORK, int i; printf("exec(");
      for (i=0; argv[i]!=0; i++)
          printf("%s%s", argv[i], (argv[i+1]!=0)?", ":")\n");)
  h->pid=os9exec(os9forkc,argv[0],argv,environ,0,0,3);
  if(h->pid==-1){
    int help;
    help=errno;
    free_handler_resources(h);
    return(help);
  }
  return(0);
}

int startnullhndlr(i, name)
int i;
char *name;
{
  struct handlerresc *h;
  char *argv[4];

  T(startnullhndlr)
  h= &hresc[i];
  h->evid=_ev_creat(HANDLER_IDLE,0,0,name);
  if(h->evid==-1)return(errno);
  h->name=(char*)malloc(strlen(name)+1);
  strcpy(h->name,name);
  argv[0]=NULLHANDLER;
  argv[1]=name;
  argv[2]=handlercomname;
  argv[3]=0;
  /* Aufruf: hauwech name */
  D(D_FORK, int i; printf("exec(");
      for (i=0; argv[i]!=0; i++)
          printf("%s%s", argv[i], (argv[i+1]!=0)?", ":")\n");)
  h->pid=os9exec(os9forkc,argv[0],argv,environ,0,0,3);
  if(h->pid==-1){
    int help;
    help=errno;
    free_handler_resources(h);
    return(help);
  }
  return(0);
}

/*****************************************************************************/

int starthndlr(i)
int i;
{
T(starthndlr)
if (_ev_read(hresc[i].evid)!=HANDLER_IDLE)
  {
  D(D_REQ, printf("starthndlr in handler.c return(1)\n");)
  return(1);
  }
_ev_set(hresc[i].evid,HANDLER_START,0);
D(D_REQ, printf("starthndlr in handler.c return(0)\n");)
return(0);
}

/*****************************************************************************/

errcode windtape(i, offset)
int i, offset;
{
  int evid;
  T(windtape)
  evid=hresc[i].evid;
  if(evid==-1)return(Err_NoDo);
  if(_ev_read(evid)!=HANDLER_IDLE)return(Err_DataoutBusy);
  if(offset<(-MAX_WIND))offset= -MAX_WIND;
  if(offset>MAX_WIND)offset=MAX_WIND;
  _ev_set(evid,offset,0);
  return(OK);
}
/*****************************************************************************/

errcode writeoutput(i, header, data)
int i, header, *data;
{
  int evid,len;
  VOLATILE int *ptr;

  T(writeoutput)
  evid=hresc[i].evid;
  if(evid==-1)return(Err_NoDo);
  if(_ev_read(evid)!=HANDLER_IDLE)return(Err_DataoutBusy);
  if(combuf[0])return(Err_DataoutBusy);
  combuf[0]=1;
  len= *data++;
  if(header){
    combuf[1]=len+4;
    combuf[2]=len+3;
    combuf[3]=0;
    combuf[4]=0;
    combuf[5]=len;
    ptr= &combuf[6];
  }else{
    combuf[1]=len;
    ptr=&combuf[2];
  }
  while(len--)*ptr++= *data++;
  _ev_set(evid,HANDLER_WRITE,0);
  return(OK);
}

/*****************************************************************************/

errcode getoutputstatus(i)
int i;
{
  int evid,len,to;
  VOLATILE int *ptr;

  T(getoutputstatus)
  evid=hresc[i].evid;
  if(evid==-1)return(Err_NoDo);
  if(_ev_read(evid)!=HANDLER_IDLE)return(Err_DataoutBusy);
  if(combuf[0])return(Err_DataoutBusy);
  combuf[0]=1;
  _ev_set(evid,HANDLER_GETSTAT,0);
  to=300;
  while((combuf[0])&&(--to))tsleep(2);
  if(!to){
    combuf[0]=0;
    return(Err_DataoutBusy);
  }
  len=combuf[1];
  ptr= &combuf[2];
  while(len--)*outptr++= *ptr++;
  return(OK);
}
/*****************************************************************************/

errcode enableoutput(i)
int i;
{
  int evid;
  T(enableoutput)
  evid=hresc[i].evid;
  if(evid==-1)return(Err_NoDo);
  if(_ev_read(evid)!=HANDLER_IDLE)return(Err_DataoutBusy);
  _ev_set(evid,HANDLER_ENABLE,0);
  return(OK);
}
/*****************************************************************************/

int disableoutput(int i)
{
  int evid;
  T(enableoutput)
  evid=hresc[i].evid;
  if(evid==-1)return(Err_NoDo);
  if(_ev_read(evid)!=HANDLER_IDLE)return(Err_DataoutBusy);
  _ev_set(evid,HANDLER_DISABLE,0);
  return(OK);
}

/*****************************************************************************/
int flushoutput(ems_u32 i)
{
    T(flushoutput)
    /* there is nothing to flush in ringbuffers */
    return OK;
}
/*****************************************************************************/

int stophndlr(i,timeout)
int i,timeout;
{
  struct handlerresc *h;
  int alm;
  unsigned int status;

  T(stophndlr)
  D(D_REQ, printf("stophndlr(%d ,%d)\n", i, timeout);)
  h= &hresc[i];
  if(_ev_read(h->evid)!=HANDLER_IDLE){
    kill(h->pid,5);
  }else{
    _ev_set(h->evid,HANDLER_QUIT,0);
  }
  alm=alm_set(SIGWAKE,timeout*100);
  if(wait(&status)){
    alm_delete(alm);
  }else{
    alm=alm_set(SIGWAKE,500);
    kill(h->pid,SIGQUIT);
    if(wait(&status)){
      alm_delete(alm);
    }else{
      return(1);
    }
  }
  free_handler_resources(h);
  return(0);
}

/*****************************************************************************/

static modresc commod;

errcode init_dataout_handler()
{
  int i;

  T(init_comm_modul)
  snprintf(handlercomname, 256, "%s.%d", HANDLERCOMNAME, getpid();
  combuf=init_datamod(handlercomname,HANDLERCOMSIZE,&commod);
  if(!combuf){
    *outptr++=errno;
    return(Err_System);
  }
  combuf[0]=0;
  for(i=0;i<MAX_DATAOUT;i++){
  struct handlerresc *h;
  h= &hresc[i];
    h->pid= -1;
    h->evid= -1;
    h->sig= -1;
    h->name=(char*)0;
  }
  outputerror_occured=0;
  return(OK);
}

/*****************************************************************************/

errcode done_dataout_handler()
{
done_datamod(&commod);
return(OK);
}

/*****************************************************************************/

/*
 * void get_dataout_list(){
 *   int i;
 *   int *help;
 *   help=outptr++;
 *   for(i=0;i<MAX_DATAOUT;i++)
 *     if(hresc[i].evid!=-1)*outptr++=i;
 *   *help=outptr-help-1;
 * }
 */

/*****************************************************************************/
/*****************************************************************************/
