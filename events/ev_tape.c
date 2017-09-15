/******************************************************************************
*                                                                             *
* ev_tape.c                                                                   *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* created before 23.03.94                                                     *
* last changed 20.01.95                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include "getevent.h"

#define RECORDMAX (4*MAXEVENT)
static int eventbuf[MAXEVENT];

static int blocksize=0, idx=0, maxidx, last_nr;
char grund[256];

/******************************************************************************

Eventformat:
0:  size
1:  Eventnummer
2:  Trigger-id
3:  Anzahl der Subevents
4:  Anfang des ersten Subevents

******************************************************************************/

int readrecord(int path, int* buf)
{
int res;

res=read(path, (char*)buf, RECORDMAX);
if (res==-1)
  {
  perror("getevent: read");
  return(-1);
  }
else
  return(res);
}

/*****************************************************************************/

int* getevent(int path)
{
int *event, *ptr;
int cblocksize, i, wenden;

if (idx>=blocksize)
  {
  if ((cblocksize=readrecord(path, eventbuf))<=0) return(0);
    if (cblocksize%4!=0)
    {
    sprintf(grund, "wrong blocksize: %d\n", cblocksize);
    errno=0;
    return(0);
    }
  blocksize=cblocksize/4;
  if (eventbuf[3]==0)
    wenden=eventbuf[0] & 0xffff0000;
  else
    wenden=eventbuf[3] & 0xffff0000;

  if (wenden)
    {
    ptr=eventbuf;
    for (i=0; i<blocksize; i++, ptr++) *ptr=ntohl(*ptr);
    }

  idx=0;
  }
event=eventbuf+idx;
idx+=event[0]+1;
return(event);
}

/*****************************************************************************/

static int* getevent_loop(int path)
{
int *event, *ptr;
int cblocksize, i, wenden;

if (idx>=blocksize)
  {
  if ((cblocksize=readrecord(path, eventbuf))<=0) return(0);
    if (cblocksize%4!=0)
    {
    sprintf(grund, "wrong blocksize: %d\n", cblocksize);
    errno=0;
    return(0);
    }
  blocksize=cblocksize/4;
  maxidx=blocksize-1;
  if (eventbuf[3]==0)
    wenden=eventbuf[0] & 0xffff0000;
  else
    wenden=eventbuf[3] & 0xffff0000;

  if (wenden)
    {
    ptr=eventbuf;
    for (i=0; i<blocksize; i++, ptr++) *ptr=ntohl(*ptr);
    }

  idx=0;
  }
event=eventbuf+idx;
idx+=event[0]+1;
if (idx>maxidx+1)
  {
  printf("getevent: event %d+1; wrong eventsize: +%d\n", last_nr, idx-maxidx);
  return((int*)-1);
  }
last_nr=event[1];
return(event);
}

/*****************************************************************************/

int* getevent_s(int path)
{
int *event;
while ((event=getevent_loop(path))==(int*)-1);
return(event);
}

/*****************************************************************************/
/*****************************************************************************/
