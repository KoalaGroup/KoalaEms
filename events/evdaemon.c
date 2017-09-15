/******************************************************************************
*                                                                             *
*  evdaemon.c                                                                 *
*                                                                             *
*  created 08.12.96                                                           *
*  last changed 08.12.96                                                      *
*                                                                             *
*  PW                                                                         *
*                                                                             *
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/param.h>

#define BUFSIZE 102400

/*****************************************************************************/

main(int argc, char* argv[])
{
FILE* con;
int fd, ok;
struct timeval tv;
struct tm* tm;
char name[MAXPATHLEN];
char buffer[BUFSIZE];

if (argc!=2)
  {
  fprintf(stderr, "usage: %s <directory>\n", argv[0]);
  exit(1);
  }

con=fopen("/dev/console", "w+");
gettimeofday(&tv, 0);
tm=localtime(&tv.tv_sec);
if (chdir(argv[1])!=0)
  {
  fprintf(con, "chdir to %s: %s\n", argv[1], strerror(errno));
  exit(1);
  }
strftime(name, MAXPATHLEN, "%d.%b.%Y_%H:%M:%S_XXXXXX", tm);
mktemp(name);
fprintf(con, "%s: %s/%s\n", argv[0], argv[1], name);
if ((fd=open(name, O_WRONLY|O_CREAT, 0644))<0)
  {
  fprintf(con, "evdaemon: open: %s\n", strerror(errno));
  fprintf(con, "evdaemon: fd=%d\n", fd);
  exit(2);
  }
ok=1;
do
  {
  int res=read(0, buffer, BUFSIZE);
  if (res>0)    
    {
    int r1=write(fd, buffer, res);
    if (r1!=res)
      {
      fprintf(con, "evdaemon: write: res=%d; %s\n", r1, strerror(errno));
      ok=0;
      }
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
fclose(con);
}
