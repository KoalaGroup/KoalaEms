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

  if (xread(s, (char*)buf, 2*sizeof(int))<0) return;
  switch (buf[1])
    {
    case 0x12345678: wenden=0; break;
    case 0x78563412: wenden=1; break;
    default: printf("wrong endien: 0x%08x\n", buf[1]); return;
    }
  if (wenden)
    len=swap_bytes(buf[0]);
  else
    len=buf[0];
  if (xread(s, (char*)(buf+2), (len-1)*sizeof(int))<0) return;
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
