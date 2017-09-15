/******************************************************************************
*                                                                             *
* evsave_t.c                                                                  *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* created 21.01.95                                                            *
* last change 21.01.95                                                        *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <unistd.h>
#include <getevent.h>

#define DEFFILE "/dev/rmt0h"

extern char grund[];

/*****************************************************************************/

int debug=0;
int verbose=0;
char* name=0;
int outfile;
int written;
int buffer[MAXEVENT];

/*****************************************************************************/

char *usage[]=
  {
  "usage: %s [-d] [-v] [-f] file\n",
  "\n",
  NULL,
  };

char optstring[]="dvf:h";

/*****************************************************************************/

void printusage(char *name)
{
int i;

i=0;
while (usage[i]!=NULL)
  {
  printf(usage[i], name);
  i++;
  }
}

/*****************************************************************************/

int readargs(int argc, char *argv[])
{
int optchar;
extern int optind;
extern char *optarg;
int optfail=0;

optfail=0;
while (((optchar=getopt(argc, argv, optstring))!=EOF) && !optfail)
  {
  switch (optchar)
    {
    case '?':
    case 'h': optfail=1; break;
    case 'd': debug=1; break;
    case 'v': debug=1; verbose=1; break;
    case 'f': name=optarg; break;
    default: optfail=1; break;
    }
  }
if (argv[optind]!=NULL)
  {
  name=argv[optind++];
  }
if (argv[optind]!=NULL)
  {
  printf("argument \"%s\" not expected\n", argv[optind]);
  optfail=1;
  }
if (!optfail && (name==0)) name=DEFFILE;
if (optfail)
  {
  int i;
  for (i=0; i<argc; i++) printf("%s%s", argv[i], i+1<argc?", ":"\n");
  printusage(argv[0]);
  return(-1);
  }
return(0);
}

/*****************************************************************************/

void do_events(int in, int out)
{
int *event, *next;
int size, eventnum, i;
int nextlog;

next=buffer; size=0; written=0;
nextlog=10000000;

for (;;)
  {
  if ((event=getevent(in))==0)
    {
    if (debug)
      {
      printf("got no event: %s, last event: %d\n", grund, eventnum);
      }
    return;
    }
  eventnum=event[1];
  if (((eventnum%1000)==0) && debug && verbose)
    {
    printf("event %d, size=%d\n", eventnum, size);
    }
  if ((size+event[0]+1)<MAXEVENT)
    {
    for (i=0; i<=event[0]; i++) *next++=event[i];
    size+=event[0]+1;
    }
  else
    {
    if (debug && verbose)
      {
      printf("write block, size=%d, event %d\n", size, eventnum);
      }
    if (write(out, (char*)buffer, 4*size)==-1)
      {
      perror("write");
      return;
      }
    written+=size; next=buffer; size=0;
    if (debug && (written>nextlog))
      {
      printf("evsave_t: wrote %d words, event %d\n", written, eventnum);
      nextlog+=10000000;
      }
    }
  }
}

/*****************************************************************************/

main(int argc, char* argv[])
{
int cons;
if (readargs(argc, argv)!=0) exit(0);
if (debug)
  {
  cons=open("/dev/console", O_WRONLY, 0);
  if (cons==-1) {perror("open console"); exit(0);}
  dup2(cons, 1);
  dup2(cons, 2);
  close(cons);
  printf("start evsave_t: verbose=%d, file=>%s<\n", verbose, name);
  }

if ((outfile=open(name, O_WRONLY, 0))==-1)
  {
  perror("open outfile");
  exit(0);
  }

do_events(0, outfile);

close(outfile);
if (debug)
  {
  printf("stop evsave_t\n");
  close(cons);
  }
}

/*****************************************************************************/
/*****************************************************************************/
