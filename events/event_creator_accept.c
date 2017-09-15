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
  if (!len)
    {
    fprintf(stderr, "len=0\n");
    len++;
    }
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
struct sockaddr_in sa;
int sock, ns, len;

if ((sock=socket(AF_INET, SOCK_STREAM, 0))==-1)
  {
  perror("socket");
  exit(0);
  }

memset((char*)&sa, 0, sizeof(struct sockaddr_in));
sa.sin_family=AF_INET;
sa.sin_port=htons(argc>1?atoi(argv[1]):8000);
sa.sin_addr.s_addr=htonl(INADDR_ANY);

if ((bind(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)))<0)
  {
  perror("bind");
  return 1;
  }

if ((listen(sock, 1))<0)
  {
  perror("listen");
  return 1;
  }

len=sizeof(struct sockaddr_in);
ns=accept(sock, (struct sockaddr*)&sa, &len);
if (ns<0)
  {
  perror("accept");
  return 1;
  }
else
  {
  printf("connected\n");
  }

send_it(ns);
}
/******************************************************************************/
