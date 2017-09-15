/******************************************************************************
*                                                                             *
* eventsplit.c                                                                *
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
char s[1024];
FILE *f0, *f1;
int *event;
int filnum;

if (argc!=2) {printf("usage: %s filename\n\n", argv[0]); exit(1);}

f0=fopen(argv[1], "r");
if (f0==0) {perror("fopen quellfile"); exit(1);}

f1=0;
filnum=0;

while (event=getevent(f0))
  {
  if ((event[1]==1) || (filnum==0))
    {
    if (f1) fclose(f1);
    sprintf(s, "%s_%03d", argv[1], ++filnum);
    f1=fopen(s, "w");
    if (f1==0) {perror("fopen zielfile"); exit(1);}
    }
  if (fwrite(event, 4, event[0]+1, f1)!=event[0]+1)
    {
    perror("fwrite");
    exit(1);
    }
  }
fclose(f0);
if (f1) fclose(f1);
}

/*****************************************************************************/
/*****************************************************************************/

