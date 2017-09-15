/*
 * events++/evdiscard.c
 * $ZEL: evdiscard.c,v 1.2 2004/02/11 12:36:06 wuestner Exp $
 * 
 * created 22.05.97 PW
 * last changed 22.05.97 PW
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>

#define BUFSIZE 102400

/*****************************************************************************/

main(int argc, char* argv[])
{
FILE* con;
int ok;
struct timeval tv;
struct tm* tm;
char buffer[BUFSIZE];
char s[1024];
int val;
size_t size;

if (argc!=1)
  {
  fprintf(stderr, "usage: %s\n", argv[0]);
  exit(1);
  }

con=fopen("/dev/console", "w+");

size=sizeof(val);
if (getsockopt(0, SOL_SOCKET, SO_RCVBUF, &val, &size)<0)
  {
  fprintf(con, "getsockopt: %s\n", strerror(errno));
  printf("getsockopt: %s\n", strerror(errno));
  exit(1);
  }
else
  {
  fprintf(con, "old buffer=%d, size=%d\n", val, size);
  printf("old buffer=%d\n", val);
  }
val*=2;
if (setsockopt(0, SOL_SOCKET, SO_RCVBUF, &val, sizeof(val))<0)
  {
  fprintf(con, "setsockopt: %s\n", strerror(errno));
  exit(1);
  }
if (getsockopt(0, SOL_SOCKET, SO_RCVBUF, &val, &size)<0)
  {
  fprintf(con, "getsockopt: %s\n", strerror(errno));
  exit(1);
  }
else
  {
  fprintf(con, "new buffer=%d\n", val);
  }

gettimeofday(&tv, 0);
tm=localtime(&tv.tv_sec);
strftime(s, 1024, "%d.%b.%Y_%H:%M:%S", tm);
fprintf(con, "evdiscard start: %s\n", s);
ok=1;
do
  {
  int res=read(0, buffer, BUFSIZE);
  if (res<0)
    {
    switch (errno)
      {
      case EINTR: break;
      default:
        fprintf(con, "evdiscard read: %s\n", strerror(errno));
        ok=0;
        break;
      }
    }
  else if (res==0)
    {
    gettimeofday(&tv, 0);
    tm=localtime(&tv.tv_sec);
    strftime(s, 1024, "%d.%b.%Y_%H:%M:%S", tm);
    fprintf(con, "evdiscard stop: %s\n", s);
    ok=0;
    }
  }
while(ok);
fclose(con);
}
