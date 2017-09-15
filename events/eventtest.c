/******************************************************************************
*                                                                             *
* eventtest.c                                                                 *
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
#include <getevent.h>

/*****************************************************************************/

main(int argc, char *argv[])
{
int f;
int *event;

if (argc!=2) {printf("usage: %s filename\n\n", argv[0]); exit(1);}

f=open(argv[1], O_RDONLY, 0);
if (f==-1) {perror("open"); exit(1);}
while (event=getevent(f))
  {
  if (event[1]%1000==0)
  printf("Event %6d: trigger %d, %d subevents, size=%d\n", event[1], event[2],
    event[3], event[0]);
  }
close(f);
}

/*****************************************************************************/
/*****************************************************************************/

