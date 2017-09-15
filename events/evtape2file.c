/******************************************************************************
*                                                                             *
* evtape2file.c                                                               *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* 20.01.95                                                                    *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getevent.h>
#include <getsubevent.h>

/*****************************************************************************/

main(int argc, char *argv[])
{
int fi;
FILE* fo;
int *event;
int is;
int size;
int *subevent;
struct
  {
  int size;
  int num;
  int t_id;
  int nsub;
  } evheader;

int lastsub, lastev, lastfalse;

if ((argc!=2) && (argc!=3))
    {printf("usage: %s filename [is_id]\n\n", argv[0]); exit(1);}
if (argc==3)
  {
  int res=sscanf(argv[2], "%d", &is);
  if (res!=1)
    {
    printf("cannot convert is_id\n");
    exit(1);
    }
  printf("is=%d\n", is);
  lastsub=0;
  }
else
  is=0;

fi=open("/dev/rmt0h", O_RDONLY, 0);
if (fi==-1)
  {
  perror("open tape");
  exit(1);
  }
fo=fopen(argv[1], "w");
if (fo==0) {perror("open file"); exit(1);}
lastev=0;
while (event=getevent_s(fi))
  {
  if (event[1]==0)
    {
    printf("userrecord nach event %d\n", lastev);
    }
  else
    {
    if ((event[0]<0) || (event[0]>65535))
      {
      printf("falsche Eventgroesse: %d\n", event[0]);
      goto fehler;
      }
    if ((event[1]<=lastev) && (event[1]!=lastfalse+1))
      {
      printf("falsche Eventnummer %d, %d erwartet.\n", event[1], lastev+1);
      lastfalse=event[1];
      goto fehler;
      }
    lastev=event[1];
    if (event[1]%10000==0)
        printf("Event %6d: trigger %d, %d subevents, size=%d\n", event[1],
        event[2], event[3], event[0]);
    if (is)
      {
      if (subevent=getsubevent_s(event, is))
        {
        evheader.size=subevent[1]+5;
        evheader.num=event[1];
        evheader.t_id=event[2];
        evheader.nsub=1;
        if (fwrite(&evheader, 4, 4, fo)!=4)
          {
          perror("fwrite"); exit(1);
          }
        if (fwrite(subevent, 4, subevent[1]+2, fo)!=subevent[1]+2)
          {
          perror("fwrite"); exit(1);
          }
        lastsub=event[1];
        }
      else
        {
        if ((event[1]-lastsub)>100)
          {
          printf("no subevent %d\n", is);
          lastsub=event[1];
          }
        }
      }
    else
      {
      if (fwrite(event, 4, event[0]+1, fo)!=event[0]+1)
        {
        perror("fwrite");
        exit(1);
        }
      }
    fehler:;
    }
  }
close(fi);
fclose(fo);
}

/*****************************************************************************/
/*****************************************************************************/
