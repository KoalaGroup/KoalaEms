/******************************************************************************
*                                                                             *
* eventftest.c                                                                *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* 23.03.94                                                                    *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getfevent.h>

/*****************************************************************************/

main(int argc, char *argv[])
{
FILE* f;
int *event;
int evnum, first, numevents;

if (argc!=2) {printf("usage: %s filename\n\n", argv[0]); exit(1);}

f=fopen(argv[1], "r");
if (f==0) {perror("fopen"); exit(1);}

evnum=-1; first=1; numevents=0;
while (event=getevent(f))
  {
  if (event[1]==0)
    {
    printf("userrecord\n");

    }
  else
    {
    numevents++;
    if (first)
      {
      evnum=event[1];
      printf("erstes Event: %6d; %d subevents, size=%d\n",
          event[1], event[3], event[0]);
      first=0;
      }
    else
      {
      if (event[1]!=++evnum)
        {
        if (event[1]==evnum+1)
          {
          printf("Event  %d fehlt.\n", evnum);
          }
        else if (event[1]>evnum)
          {
          printf("Events %d bis %d fehlen.\n", evnum, event[1]-1);
          }
        else
          {
          printf("Sprung von Event %d zu %d.\n", evnum-1, event[1]);
          }
        evnum=event[1];
        }
      }
    }
  }
if (evnum==-1)
  printf("kein Event.\n");
else
  printf("letztes Event: %6d, gesamt %d Events.\n", evnum, numevents);
fclose(f);
}

/*****************************************************************************/
/*****************************************************************************/
