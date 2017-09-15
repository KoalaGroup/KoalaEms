/*
 * commu_anno.cc
 * 
 * created before 25.07.94 PW
 */

#include "config.h"
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h> /* wegen struct iovec */
#include <socket_prot.h> /* sys/socket.h, aber geschuetzt (wegen ultrix4.4) */
#include <netinet/in.h>
#include <netdb.h>
#include <commu_anno.hxx>
#include <xdrstring.h>
#include <compat.h>
#include "versions.hxx"

VERSION("Jul 25 1994", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_anno.cc,v 2.6 2004/11/26 15:14:14 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

extern char *sockname;
extern short port;

/*****************************************************************************/
#define VERSION 2

int hosts[]={0x10c85e86, 0x11c85e86, 0x47c85e86, 0x5cc85e86, 0xa5c85e86, 0};

void C_announce::do_it(int val)
{
int array[300];
char name[1024];
struct hostent *host;
int *ptr;
int sock, iport, i;
int size;
size_t tolen;
struct sockaddr_in to;

array[0]=htonl(VERSION);
array[1]=htonl(val);
array[2]=htonl(getpid());
array[3]=htonl(getuid());
array[4]=htonl(getgid());

array[5]=0;
if (gethostname(name, 256)==0)
  {
  host=gethostbyname(name);
  if (host!=0)
    array[5]=*(int*)(host->h_addr);
  }
iport=port;
array[6]=htonl(iport);
ptr=array+7;
ptr=outstring(ptr, sockname);
ptr=outstring(ptr, name);
ptr=outstring(ptr, getenv("USER"));
ptr=outstring(ptr, getenv("HOME"));
getwd(name);
ptr=outstring(ptr, name);

size=(ptr-array)*4;
sock=socket(AF_INET, SOCK_DGRAM, 0);
if (sock==-1) return;
bzero((char*)&to, sizeof(struct sockaddr_in));
to.sin_family=AF_INET;
to.sin_port=htons(6009);

for (i=0; hosts[i]!=0; i++)
  {
  to.sin_addr.s_addr=hosts[i];
  tolen=sizeof(struct sockaddr_in);
  sendto(sock, (char*)array, size, 0, (struct sockaddr*)&to, tolen);
  }
close(sock);
}

static C_announce anno;

/*****************************************************************************/
/*****************************************************************************/
