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
unsigned int swap_bytes (unsigned int n)
{
return ((n & 0xff000000) >> 24) |
       ((n & 0x00ff0000) >>  8) |
       ((n & 0x0000ff00) <<  8) |
       ((n & 0x000000ff) << 24);
}
/******************************************************************************/
int xread(int p, char* b, int s)
{
int r, d=0;
while (d<s)
  {
  r=read(p, b+d, s-d);
  if (r<0)
    {
    perror("read");
    return -1;
    }
  if (r==0)
    {
    fprintf(stderr, "broken pipe.\n");
    return -1;
    }
  d+=r;
  }
return 0;
}
/******************************************************************************/
void read_it(int s)
{
int* buf;
int len, i, x;

buf=malloc(0x4000*sizeof(int));
if (buf==0)
  {
  perror("malloc");
  return;
  }

for (i=0; i<0x4000; i++) buf[i+1]=i;

while(1)
  {
  int wenden;

  if (xread(s, (char*)buf, sizeof(int))<0) return;
  wenden=buf[0]&0xffff0000;
  if (wenden)
    len=swap_bytes(buf[0]);
  else
    len=buf[0];
  if (xread(s, (char*)(buf+1), len*sizeof(int))<0) return;
  if (wenden) for (i=0; i<=len; i++) buf[i]=swap_bytes(buf[i]);
  if (buf[1]%10000==0) printf("event %d\n", buf[1]);
/*
 *   x=buf[1]%100==0;
 *   if (x) fprintf(stderr, "got %d [%d]\n", buf[1], len);
 *   for (i=2; i<=len; i++)
 *     {
 *     if (buf[i]!=i)
 *       {fprintf(stderr, "error at %d: 0x%08x --> 0x%08x\n",
 *         i, i, buf[i]);
 *       return;
 *       }
 *     }
 */
  }
}
/******************************************************************************/
main(int argc, char **argv)
{
char* hostname;
struct hostent *host;
struct sockaddr_in sa;
int sock;

if ((sock=socket(AF_INET, SOCK_STREAM, 0))==-1)
  {
  perror("socket");
  exit(0);
  }
else
  {
  printf("sock=%d\n", sock);
  }

if (argc>1)
  hostname=argv[1];
else
  hostname="localhost";

memset((char*)&sa, 0, sizeof(struct sockaddr_in));
sa.sin_family=AF_INET;
sa.sin_port=htons(argc>2?atoi(argv[2]):8000);
host=gethostbyname(hostname);
if (host!=0)
  {
  sa.sin_addr.s_addr= *(u_int*)(host->h_addr_list[0]);
  }
else
  {
  sa.sin_addr.s_addr=inet_addr((char*)hostname);
  if (sa.sin_addr.s_addr==-1)
    {
    printf("komischer Hostname: %s\n", hostname);
    exit(0);
    }
  }

if (connect(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))==-1)
  {
  perror("connect");
  exit(0);
  }
read_it(sock);
}
/******************************************************************************/
