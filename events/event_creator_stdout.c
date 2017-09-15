#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

/******************************************************************************/
int xsend(int p, char* b, int s)
{
int r, d=0;
while (d<s)
  {
  r=write(p, b+d, s-d);
  if (r<0)
    {
    perror("write");
    return -1;
    }
  if (r==0)
    {
    fprintf(stderr, "hae?\n");
    return -1;
    }
  d+=r;
  }
return 0;
}
/******************************************************************************/
void send_it(int s)
{
int* buf;
int len, i, n=0, x;

srandom(17);

buf=malloc(0x4000*sizeof(int));
if (buf==0)
  {
  perror("malloc");
  return;
  }

for (i=0; i<0x4000; i++) buf[i]=i;

while(1)
  {
  len=random()&0x3fff;
  if (!len) len++;
  x=n%100==0;
  if (x) fprintf(stderr, "%d [%d]", buf[1], buf[0]);
  buf[0]=len;
  buf[1]=n++;
  if (xsend(s, (char*)buf, (len+1)*sizeof(int))<0) return;
  if (x) fprintf(stderr, "<\n");
  }
}
/******************************************************************************/
main(int argc, char **argv)
{
send_it(1);
}
/******************************************************************************/
