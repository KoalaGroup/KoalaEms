/******************************************************************************
*                                                                             *
* ev_file.c                                                                   *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* 23.03.94                                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include "getevent.h"

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

static int xrecv(int path, int* buf, int len)
{
int da, restlen;

restlen=len*sizeof(int);
if ((da=read(path, (char*)buf, restlen))!=restlen)
  {
  if (da==0)
    {
    strcpy(grund, "no more data");
    errno=0;
    }
  else
    {
    sprintf(grund, "path=%d, len=%d, da=%d, errno=%d", path, len, da, errno);
    }
  return(-1);
  }

return(0);
}

/*****************************************************************************/

int* getevent(int path)
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
