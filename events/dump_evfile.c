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
#include <getfevent.h>
#include <getsubevent.h>

/*****************************************************************************/

main(int argc, char *argv[])
{
FILE* fi;
int *event;
int size, numis;
int *subevent;

int lastev, lastfalse;

if (argc!=2)
  {
  printf("usage: %s filename\n", argv[0]);
  exit(0);
  }
fi=fopen(argv[1], "r");
if (fi==0)
  {
  perror("fopen");
  exit(1);
  }
lastev=0;
while (event=getevent(fi))
  {
  size=event[0];
  printf("%04d: %04d", event[1], size); fflush(stdout);
  if (event[1]==0)
    {
    printf("userrecord\n");
    }
  else
    {
    if ((event[0]<0) || (event[0]>65535))
      {
      printf("falsche Eventgroesse\n");
      goto fehler;
      }
    if (((event[1]<=lastev) || (event[1]>500000)) && (event[1]!=lastfalse+1))
      {
      printf("falsche Eventnummer %d, %d erwartet.\n", event[1], lastev+1);
      lastfalse=event[1];
      goto fehler;
      }
    lastev=event[1];
    numis=event[3];
    printf(" %2d %2d\n", event[2], numis);
    fehler:;
    }
  }
fclose(fi);
}

/*****************************************************************************/
/*****************************************************************************/
