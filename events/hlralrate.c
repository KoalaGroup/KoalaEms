#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int data[1024];
char str[1024];
int arr[1024];
int narr[1024];
int s;
struct timeval start, now;

int xread(int s, char* buf, int count)
{
int idx=0, res, i;
while (count)
  {
  res=read(s, buf+idx, count);
  if (res<0)
    {
    if (errno!=EINTR)
      {
      fprintf(stderr, "xread: %s", strerror(errno));
      return -1;
      }
    }
  else if (res==0)
    {
    fprintf(stderr, "xread: read 0 bytes\n");
    return -1;
    }
  else
    {
    idx+=res;
    count-=res;
    }
  }
/*
 * fprintf(stderr, "xread(%d): ", idx);
 * for (i=0; i<idx; i++) fprintf(stderr, "0x%02x ", buf[i]);
 * fprintf(stderr, "\n");
 */
return 0;
}

int send_str(char* str)
{
/*fprintf(stderr, "send: >%s<\n", str);*/
return write(s, str, strlen(str));
}

int send_name(int n)
{
int col, row, chan;
int code;

col=(n>>16)&0xff;
row=(n>>8)&0xff;
chan=n&0xff;

sprintf(str, "{name chan_%d {(%d %d %d)}}\n", n, col, row, chan);
return send_str(str);
}

int send_data(int n, int val)
{
sprintf(str, "{value chan_%d %d. %d.}\n", n, now.tv_sec, val);
return send_str(str);
}

int open_socket(char* hostname, int port)
{
struct hostent *host;
struct sockaddr_in sa;
int sock;

if ((sock=socket(AF_INET, SOCK_STREAM, 0))==-1)
  {
  perror("socket");
  return -1;
  }

memset((char*)&sa, 0, sizeof(struct sockaddr_in));
sa.sin_family=AF_INET;
sa.sin_port=htons(port);
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
    fprintf(stderr, "komischer Hostname: %s\n", hostname);
    return -1;
    }
  }
if (connect(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))==-1)
  {
  perror("connect");
  return -1;
  }
return sock;
}

main(int argc, char* argv[])
{
int n;

if (argc!=3)
  {
  fprintf(stderr, "usage: %s host port\n", argv[0]);
  return 1;
  }
s=open_socket(argv[1], atoi(argv[2]));
if (s<0) return 2;

send_str("master hlralrate\n");

for (n=0; n<1024; n++) {arr[n]=0; narr[n]=0;}
gettimeofday(&start, 0);
do
  {
  int size;
  if (xread(0, (char*)&size, sizeof(int))) return 3;
/*
 *   fprintf(stderr, "size=%d\n", size);
 */
  if (xread(0, (char*)data, size*sizeof(int))) return 4;
  /*fprintf(stderr, "."); fflush(stderr);*/
  for (n=0; n<size; n++)
    {
    int code=data[n];
/*
 *     fprintf(stderr, "code[%d]=0x%x\n", n, code);
 */
    if ((unsigned int)code<1024)
      arr[code]++;
    else
      fprintf(stderr, "got channel %d\n", code);
    }
  gettimeofday(&now, 0);
  if (now.tv_sec>start.tv_sec)
    {
    for (n=0; n<1024; n++)
      {
      if (arr[n])
        {
        if (!narr[n]) {send_name(n); narr[n]=1;}
        send_data(n, arr[n]);
        arr[n]=0;
        }
      }
    start=now;
    }
  }
while (1);
}
