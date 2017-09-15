/*
 * events++/evslowdaemon.c
 * $ZEL: evslowdaemon.c,v 1.7 2006/02/17 15:17:08 wuestner Exp $
 * 
 * created 02.04.98 PW
 * last changed 02.04.98 PW
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#ifndef __linux__
#include <sys/timers.h>
#endif
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <math.h>
#define BUFSIZE 4194304

struct timeval start;
int bytes;
float speed;
FILE* con;

/*****************************************************************************/
void slow_down()
{
struct timeval jetzt;
float zeit, kBps;
int abytes;

gettimeofday(&jetzt, 0);
zeit=(float)(jetzt.tv_sec-start.tv_sec)
    +(float)(jetzt.tv_usec-start.tv_usec)/1000000.;
/*
kBps=bytes/zeit/1024.;
if (speed>=kBps) return;
*/
abytes=speed*zeit*1024;
if (bytes>abytes)
  {
  struct timespec rqtp, rmtp;
  float i, f;
  float wzeit=(float)(bytes-abytes)/speed/1024.;
  f=modff(wzeit, &i);
  rqtp.tv_sec=i;
  rqtp.tv_nsec=f*1000000000.;
  nanosleep(&rqtp, &rmtp);
  }
}
/*****************************************************************************/

main(int argc, char* argv[])
{
int fd, ok;
struct timeval tv;
struct tm* tm;
char name[MAXPATHLEN];
char buffer[BUFSIZE];
char* dir;
{
extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;
int c, errflg=0;

speed=0;
optarg=NULL;
while (!errflg && ((c=getopt(argc, argv, "s:"))!=-1))
  switch (c)
    {
    case 's':
      speed=atof(optarg);
      break;
    default :
      errflg++;
    }

if (errflg)
  {
  fprintf(stderr, "usage: %s [-s speed/(KByte/s)] [directory]\n", argv[0]);
  exit(1);
  }
if (optind<argc)
  dir=argv[optind];
else
  dir=0;
}

con=fopen("/var/log/evslowdaemon", "w+");
if (!con) con=stderr;
if (dir)
  {
  gettimeofday(&tv, 0);
  tm=localtime(&tv.tv_sec);
  if (chdir(dir)!=0)
    {
    fprintf(con, "chdir to %s: %s\n", dir, strerror(errno));
    exit(1);
    }
  strftime(name, MAXPATHLEN, "%d.%b.%Y_%H:%M:%S_XXXXXX", tm);
  mkstemp(name);
  fprintf(con, "%s: %s/%s\n", argv[0], dir, name);
  if ((fd=open(name, O_WRONLY|O_CREAT, 0644))<0)
    {
    fprintf(con, "evdaemon: open: %s\n", strerror(errno));
    fprintf(con, "evdaemon: fd=%d\n", fd);
    exit(2);
    }
  fchmod(fd, 0644);
  }
else
  {
  fprintf(con, "%s: no output\n", argv[0], dir, name);
  fd=-1;
  }
ok=1; bytes=0;
gettimeofday(&start, 0);
do
  {
  int res;
  if (speed>0) slow_down();
  res=read(0, buffer, BUFSIZE);
  if (res>0)    
    {
    if (fd>=0)
      {
      int r1=write(fd, buffer, res);
      if (r1!=res)
        {
        fprintf(con, "evdaemon: write: res=%d; %s\n", r1, strerror(errno));
        ok=0;
        }
      }
    bytes+=res;
    }
  else if (res<0)
    {
    switch (errno)
      {
      case EINTR: break;
      default:
        fprintf(con, "evdaemon: read: res=%d; %s\n", res, strerror(errno));
        ok=0;
        break;
      }
    }
  else
    {
    fprintf(con, "evdaemon: no more data\n");
    ok=0;
    }
  }
while(ok);
close(fd);
if (con!=stderr) fclose(con);
}
