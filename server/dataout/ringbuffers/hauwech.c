/******************************************************************************
*                                                                             *
* hauwech.c                                                                   *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 09.03.94                                                                    *
*                                                                             *
******************************************************************************/

#define READOUT_PRIOR 1000
#define HANDLERCOMNAME "handlercom"

#include <stdio.h>
#include <module.h>
#include <errno.h>

#include "handlers.h"

/*****************************************************************************/

static int evid,error,fertig,*buffer;
static mod_exec *head;
static VOLATILE int *ip;

/*****************************************************************************/

void cleanup()
{
if (evid!=-1) _ev_unlink(evid);
if (head!=(mod_exec*)-1) munlink(head);
exit(0);
}

/*****************************************************************************/

void liesringbuffer()
{
    register int len;
    if(!(*ip)){
#ifndef NOSMARTPOLL
	tsleep(1);
#endif
	return;
    }
    len= *(ip+1);
    *ip=0;
    if(len>-1){
	ip+=len+2;
    }else{
	if(len==-2)fertig=2;
	else ip=buffer;
    }
}

/*****************************************************************************/

main(argc, argv)
int argc; char **argv;
{
char *handlercomname;
int was;

if(argc<2)exit(0);

error=0;

head=(mod_exec*)modlink(argv[1], mktypelang(MT_DATA, ML_ANY));
if (head==(mod_exec*)-1) error=errno;
buffer=(int*)((char*)head+head->_mexec);

evid=_ev_link(argv[1]);
if(evid==-1)exit(errno);

if(argc>2)handlercomname=argv[2];
else handlercomname=HANDLERCOMNAME;

intercept(cleanup);
fertig=1;
for(;;)
  {
  if (fertig)
    {
    was=_ev_wait(evid,HANDLER_MINVAL,HANDLER_MAXVAL);
    _ev_set(evid,HANDLER_IDLE,0);
    }
  else
    {
    int i;
    for (i=0; ((i<READOUT_PRIOR)&&(!fertig)); i++) liesringbuffer();
    was=_ev_read(evid);
    _ev_set(evid,HANDLER_IDLE,0);
    }
  if (was==HANDLER_START)
    {
    if (!error)
      {
      fertig=0;
      ip=buffer;
      }
    }
  else if (was==HANDLER_QUIT)
    {
    while (!(error||fertig)) liesringbuffer();
    cleanup();
    }
  else if (was==HANDLER_GETSTAT)
    {
    mod_exec *hmod;
    int *buf;
    hmod=(mod_exec*)modlink(handlercomname,mktypelang(MT_DATA,ML_ANY));
    if (hmod==(mod_exec*)-1) exit(errno);
    buf=(int*)((char*)hmod+hmod->_mexec);
    buf[1]=3;
    buf[2]=error;
    buf[3]=fertig;
    buf[4]= -1;
    buf[0]=0;
    munlink(hmod);
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/

