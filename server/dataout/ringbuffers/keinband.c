/******************************************************************************
*                                                                             *
* aufexa.c                                                                    *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 08.07.94                                                                    *
*                                                                             *
******************************************************************************/

#include <stdio.h>
#include <module.h>
#include <errno.h>
#include <types.h>
#include <modes.h>

#define READOUT_PRIOR 1000
#define MAX_BLCK 0x3f00 /* in Integern */
#define HANDLERCOMNAME "handlercom"

#include "handlers.h"

static int evid;
static mod_exec *head;

static int outbuf[MAX_BLCK];
static int cursize, *curpnt;
static int fertig, disabled, tapeerror, tapefull, *buffer;
static VOLATILE int *ip;

static int ppid, sig;

/*****************************************************************************/

void flushbuffer()
{
cursize=0;
curpnt=outbuf;
}

/*****************************************************************************/

void gib_weiter(buf,size)
int *buf,size;
{
if ((cursize+size)>MAX_BLCK)
  {
  cursize=0;
  curpnt=outbuf;
  }
memcpy(curpnt, buf, size*sizeof(int));
cursize+=size;
curpnt+=size;
}

/*****************************************************************************/

cleanup()
{
if (evid!=-1) _ev_unlink(evid);
if (head!=(mod_exec*)-1) munlink(head);
exit(0);
}

/*****************************************************************************/

void liesringbuffer()
{
register int len;
if (!(*ip))
  {
#ifndef NOSMARTPOLL
  tsleep(1);
#endif
  return;
  }
len= *(ip+1);
if (len>-1)
  {
  gib_weiter(ip+1,len+1);
  *ip=0;
  ip+=len+2;
  }
else
  {
  *ip=0;
  if (len==-2)
    fertig=2;
  else
    ip=buffer;
  }
}

/*****************************************************************************/

main(argc, argv)
int argc; char **argv;
{
char *handlercomname;
int was;

printf("keinband: start main\n");

if (argc<4) exit(0);

tapeerror=0;
disabled=0;

head=(mod_exec*)modlink(argv[2], mktypelang(MT_DATA,ML_ANY));
if (head==(mod_exec*)-1) tapeerror=errno;
buffer=(int*)((char*)head+head->_mexec);

evid=_ev_link(argv[2]);
if (evid==-1) exit(errno);

if (sscanf(argv[3],"%d",&sig)!=1) tapeerror=E_ILLARG;
ppid=getppid();

if (argc>4)
  handlercomname=argv[4];
else
  handlercomname=HANDLERCOMNAME;

intercept(cleanup);

fertig=1;
cursize=0;
curpnt=outbuf;

for (;;)
  {
  if (fertig)
    {
    printf("keinband: fertig, warte auf event\n");
    was=_ev_wait(evid, HANDLER_MINVAL, HANDLER_MAXVAL);
    _ev_set(evid, HANDLER_IDLE, 0);
    }
  else
    {
    int i;
    for (i=0; (i<READOUT_PRIOR) && !fertig; i++) liesringbuffer();
    was=_ev_read(evid);
    printf("keinband: nicht fertig, lese event.\n");
    _ev_set(evid, HANDLER_IDLE, 0);
    }

  switch (was)
    {
    case HANDLER_IDLE: break;
    case HANDLER_START:
      printf("keinband: HANDLER_START\n");
      if (!tapeerror)
        {
        fertig=0;
        ip=buffer;
        }
      break;
    case HANDLER_QUIT:
      printf("keinband: HANDLER_QUIT\n");
      while (!(tapeerror||fertig)) liesringbuffer();
      if (!(tapeerror|tapefull)) flushbuffer();
      cleanup();
      break;
    case HANDLER_GETSTAT:
      {
      mod_exec *hmod;
      int *buf;
      printf("keinband: HANDLER_GETSTAT\n");
      hmod=(mod_exec*)modlink(handlercomname,mktypelang(MT_DATA,ML_ANY));
      if (hmod==(mod_exec*)-1) exit(errno);
      buf=(int*)((char*)hmod+hmod->_mexec);
      buf[3]=fertig;
      buf[4]=disabled;
      if (tapeerror)
        {
        buf[2]=tapeerror;
        buf[1]=3;
        }
      else
        {
        buf[1]=7;
        buf[2]=0;
        buf[5]=5000000;
        buf[6]=0;
        buf[7]=1<<16;
        buf[8]=0;
        }
      buf[0]=0;
      munlink(hmod);
      }
      break;
    case HANDLER_ENABLE:
      printf("keinband: HANDLER_ENABLE\n");
      if (!tapeerror) disabled=0;
      break;
    case HANDLER_DISABLE:
      printf("keinband: HANDLER_DISABLE\n");
      disabled=1;
      if (fertig==3) fertig=0;
      break;
    case HANDLER_WRITE:
      printf("keinband: HANDLER_WRITE\n");
      {
      mod_exec *hmod;
      int *buf;
      hmod=(mod_exec*)modlink(handlercomname, mktypelang(MT_DATA,ML_ANY));
      if (hmod==(mod_exec*)-1)
        {
        tapeerror=errno;
        break;
        }
      buf=(int*)((char*)hmod+hmod->_mexec);
      flushbuffer();

      buf[0]=0;
      munlink(hmod);
      }
      break;
    case -MAX_WIND:
      printf("keinband: -MAX_WIND\n");
      flushbuffer();
      if ((!tapeerror)||(tapeerror==E_NOTRDY))
        {
        tapefull=0;
        }
      break;
    case MAX_WIND:
      printf("keinband: MAX_WIND\n");
      flushbuffer();
      break;
    case 0:
      printf("keinband: 0\n");
      flushbuffer();
      break;
    default:
      printf("keinband: default\n");
      if ((was>-MAX_WIND) && (was<MAX_WIND))
        {
        flushbuffer();
        }
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/

