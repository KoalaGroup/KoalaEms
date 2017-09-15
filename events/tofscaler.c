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

struct timeval last={0, 0};
double tdiff=0;
unsigned long long vals[6][32];
long long diffs[6][32];
/******************************************************************************/
void init()
{
int m, c;
for (m=0; m<6; m++)
  for (c=0; c<32; c++)
    {
    vals[m][c]=0;
    diffs[m][c]=0;
    }
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
void do_scaler(unsigned int* buf, int len)
{
int modules, m;

modules=*buf++;
/*printf("%d modules\n", modules);*/
for (m=0; m<modules; m++)
  {
  int channels, c;
  channels=*buf++;
  /*printf("%d channels\n", channels);*/
  for (c=0; c<channels; c++)
    {
    unsigned long long val;
    val=*buf++; val<<=32;
    val+=*buf++;
    diffs[m][c]=val-vals[m][c];
    vals[m][c]=val;
    }
  }
}
/******************************************************************************/
void print_scaler(int offs)
{
int m, k=0;
for (m=0; m<6; m++)
  {
  int c;
  for (c=0; c<32; c++)
    {
    if (diffs[m][c])
      {
      printf("[%d][%2d] %10llu ", m+offs, c, vals[m][c]);
      if ((diffs[m][c]>0) && (tdiff>0))
        printf(" %10.2f/s", diffs[m][c]/tdiff);
      else
        printf("  old val=%lld", vals[m][c]-diffs[m][c]);
      printf("%s", (k++&1)?"\n":"   ");
      }
    }
  }
if (k&1) printf("\n");
}
/******************************************************************************/
void do_latch(unsigned int* buf, int len)
{
int m, i;
m=*buf++;
printf("Latch:");
for (i=0; i<m; i++)
  {
  int n, j;
  n=*buf++;
  for (j=0; j<n; j++)
    {
    unsigned int v;
    v=*buf++;
    printf(" [%d][%d]0x%08x", i, j, v);
    }
  }
printf("\n");
}
/******************************************************************************/
void do_timestamp(unsigned int* buf, int len)
{
time_t sec;
int usec;
struct tm *tm;
char s[1024];

sec=buf[0];
usec=buf[1];
tm=localtime(&sec);
strftime(s, 1024, "%e. %b %H:%M:%S", tm);
printf("  %s", s);
if (last.tv_sec)
  {
  tdiff=sec-last.tv_sec+(usec-last.tv_usec)/1000000.;
  printf("  tdiff=%f s\n", tdiff);
  }
else
  printf("\n");
last.tv_sec=sec;
last.tv_usec=usec;
}
/******************************************************************************/
void do_subevent(int id, unsigned int* buf, int len)
{
switch (id)
  {
  case 1:
    /*printf("Timestamp\n");*/
    do_timestamp(buf, len);
    break;
  case 2:
    /*printf("Latch\n");*/
    do_latch(buf, len);
    break;
  case 3:
    /*printf("Scaler\n");*/
    do_scaler(buf, len);
    break;
  }
}
/******************************************************************************/
void do_event(unsigned int* buf, int len)
{
unsigned int* p;
int i;

printf("Event %d; %d SubEvents\n", buf[0], buf[2]);
p=buf+3;
for (i=0; i<buf[2]; i++)
  {
  /*printf("IS_ID %d, size=%d\n", p[0], p[1]);*/
  do_subevent(p[0], p+2, p[1]);
  p+=p[1]+2;
  }
}
/******************************************************************************/
void do_read(int s)
{
static unsigned int *buf=0, bufsize=0;

while (1)
  {
  int len, i;

  if (xread(s, (char*)&len, sizeof(int))<0) return;
  len=ntohl(len);
  if (bufsize<len)
    {
    free(buf);
    buf=(unsigned int*)malloc(len*sizeof(int));
    if (!buf)
      {
      perror("malloc");
      return;
      }
    }
  if (xread(s, (char*)buf, sizeof(int)*len)<0) return;
  for (i=0; i<len; i++) buf[i]=ntohl(buf[i]);
  do_event(buf, len);
  print_scaler(2);
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
sa.sin_port=htons(argc>2?atoi(argv[2]):11111);
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
init();
do_read(sock);
}
/******************************************************************************/
/******************************************************************************/
