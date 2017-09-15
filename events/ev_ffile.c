/******************************************************************************
*                                                                             *
* ev_ffile.c                                                                  *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* 23.03.94                                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include "getfevent.h"

static int eventbuf[MAXEVENT];
char grund[256];

/******************************************************************************

Eventformat:
0:  size
1:  Eventnummer
2:  Trigger-id
3:  Anzahl der Subevents
4:  Anfang des ersten Subevents

******************************************************************************/

static int xrecv(FILE* path, int* buf, int len)
{
int da;

errno=0;
if ((da=fread(buf, 4, len, path))!=len)
  {
  if (da==0)
    if (errno==0)
      strcpy(grund, "no more data");
    else
      strcpy(grund, strerror(errno));
  return(-1);
  }

return(0);
}

/*****************************************************************************/

int* getevent(FILE* path)
{
int i, wenden;
int* ptr;

if (xrecv(path, eventbuf, 4)==-1) return(0);

if (eventbuf[3]==0)
  wenden=eventbuf[0] & 0xffff0000;
else
  wenden=eventbuf[3] & 0xffff0000;

if (wenden)
  {
  ptr=eventbuf;
  for (i=0; i<4; i++, ptr++) *ptr=ntohl(*ptr);
  }

if (eventbuf[0]>MAXEVENT)
  {
  sprintf(grund, "Event zu gross: 0x%x\n", eventbuf[0]);
  errno=0;
  return(0);
  }

if (xrecv(path, eventbuf+4, eventbuf[0]-3)==-1) return(0);
if (wenden)
  for (i=4; i<=eventbuf[0]; i++, ptr++) *ptr=ntohl(*ptr);

return(eventbuf);
}

/*****************************************************************************/
/*****************************************************************************/
