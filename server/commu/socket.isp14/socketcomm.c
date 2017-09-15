/******************************************************************************
*                                                                             *
* socketcomm.c                                                                *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 23.09.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: socketcomm.c,v 1.4 2011/04/06 20:30:21 wuestner Exp $";

#include <sconf.h>
#include <types.h>
#include <in.h>
#include <socket.h>
#include <netdb.h>
#include <sgstat.h>
#include <errno.h>

#include <msg.h>
#include <requesttypes.h>
#include <intmsgtypes.h>
#include <errorcodes.h>
#include <swap.h>
#include <debug.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "")

/*****************************************************************************/

static int s, ns;
extern msgheader header;
static char newconnection, newdata;

/*****************************************************************************/

static void connsighdl()
{
newconnection=1;
}

/*****************************************************************************/

static void datasighdl()
{
newdata=1;
}

/*****************************************************************************/

errcode init_comm(port)
char *port;
{
int p;
struct sockaddr_in sin;
unsigned short optval;
struct linger ling;

T(init_comm)
p=DEFPORT;
if (port)
  if (sscanf(port, "%d", &p)!=1)
    {
    printf("Bloede Portnummer! (%s)\n", port);
    return(Err_ArgRange);
    }

D(D_COMM, printf("using socket on port %d\n", p);)
s=socket(AF_INET, SOCK_STREAM, 0);
optval=1;
setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
ling.l_onoff=1;
ling.l_linger=3;
setsockopt(s, SOL_SOCKET, SO_LINGER, &ling, sizeof(struct linger));

sin.sin_family=AF_INET;
sin.sin_port=htons(p);
sin.sin_addr.s_addr=INADDR_ANY;
if (bind(s, &sin, sizeof(sin))==-1)
  {
  printf("bind schlug fehl, errno=%d\n", errno);
  return(Err_System);
  }
listen(s, 1);

newconnection=newdata=0;
install_signalhandler(123,connsighdl);
install_signalhandler(124,datasighdl);
_ss_ssig(s,123);

ns= -1;
return(OK);
}

/*****************************************************************************/

Request get_request(reqbuf, siz)
int **reqbuf, *siz;
{
struct sockaddr_in sin;
int len, restlen, da, size, res;
char *bufptr;
int *msg;

T(get_request)
do
  {
  if (ns<0)
    {
    while (ns<0)
      {
      len=sizeof(struct sockaddr_in);
      ns=accept(s, &sin, &len);
      if(ns>=0){
    newdata=0;
    _ss_ssig(ns,124);
      }
      D(D_COMM, if (ns>=0) printf("connect\n"); else perror("accept");)
      }
    }
  restlen=sizeof(msgheader);
  bufptr=(char*)&header;
  while(restlen)
    {
    da=recv(ns, bufptr, restlen, 0);
    if(da>0)    /*falls Error, wird gepollt*/
      {
      DV(D_COMM, printf("%d Headerbytes angekommen\n",da);)
      bufptr+=da;
      restlen-=da;
      }
    }
  swap_falls_noetig(&header, sizeof(msgheader)/sizeof(int));
  *siz=size=header.size;
  DV(D_COMM, printf("Es folgen %d Bytes!\n", size*sizeof(int));)
  if (size)
    {
    msg= *reqbuf=(int*)calloc(size, sizeof(int));
    restlen=size*sizeof(int);
    bufptr=(char*)msg;
    while(restlen)
      {
      da=recv(ns, bufptr, restlen, 0);
      if(da>0)
        {
        DV(D_COMM, printf("%d Bytes angekommen\n", da);)
        bufptr+=da;
        restlen-=da;
        }
      }
    swap_falls_noetig(msg, size);
    }
  else
    msg= *reqbuf=0;
  if(header.flags&Flag_Intmsg)
    {
    header.flags|=Flag_Confirmation;
    header.size=0;
/*swap! */
    send(ns, &header, sizeof(msgheader),0);
    if(header.type==Intmsg_Close)
      {
      close(ns);
      D(D_COMM, printf("und tschuess...!\n");)
      ns= -1;
      }
    if (msg) free(msg);
    }
  }
while ((ns<0) || (header.flags&Flag_Intmsg));
if(newconnection){newconnection=0;_ss_ssig(s,123);}
if(newdata){newdata=0;_ss_ssig(s,124);}
return(header.type);
}

/*****************************************************************************/

Request get_request_noblock(reqbuf, siz)
int **reqbuf, *siz;
{
struct sockaddr_in sin;
int len, restlen, da, size;
char *bufptr;
int *msg;

T(get_request_noblock)
if(ns<0)
  {
  if (!newconnection) return(Req_Nothing);
  newconnection=0;_ss_ssig(s,123);
  len=sizeof(struct sockaddr_in);
  ns=accept(s, &sin, &len);
  if (ns<0) return(Req_Nothing);
  newdata=0;
  _ss_ssig(ns,124);
  D(D_COMM, if (ns>=0) printf("connect\n"); else perror("accept");)
  }
if (!newdata) return(Req_Nothing);
newdata=0; _ss_ssig(s, 124);
if ((da=recv(ns, (char*)&header, sizeof(msgheader), 0))<0) return(Req_Nothing);
bufptr=(char*)&header+da;
restlen=sizeof(msgheader)-da;
while(restlen)
  {
  da=recv(ns, bufptr, restlen, 0);
  if (da>0)
    {
    DV(D_COMM, printf("%d Headerbytes angekommen\n", da);)
    bufptr+=da;
    restlen-=da;
    }
  }
swap_falls_noetig(&header, sizeof(msgheader)/sizeof(int));
*siz=size=header.size;
DV(D_COMM, printf("Es folgen %d Bytes!\n", size*sizeof(int));)
if (size)
  {
  msg= *reqbuf=(int*)calloc(size, sizeof(int));
  restlen=size*sizeof(int);
  bufptr=(char*)msg;
  while (restlen)
    {
    da=recv(ns, bufptr, restlen, 0);
    if (da>0)
      {
      DV(D_COMM, printf("%d Bytes angekommen\n", da);)
      bufptr+=da;
      restlen-=da;
      }
    }
  swap_falls_noetig(msg, size);
  }
else
  *reqbuf=(int*)0;
  if (header.flags&Flag_Intmsg)
    {
    header.flags|=Flag_Confirmation;
    send(ns,&header,sizeof(msgheader),0);
    if (header.type==Intmsg_Close)
      {
      close(ns);
      ns= -1;
      }
    if (msg) free(msg);
    return (Req_Nothing);
    }
  return(header.type);
}

/*****************************************************************************/

void send_confirmation(header, confbuf, size)
msgheader *header;
int *confbuf;
int size;
{
int restlen, da;
char *bufptr;
#ifdef DEBUG
int index;
#endif
#define MAXBYTES 1024

T(send_confirmation)
if (ns<0)
  {
  D(D_COMM, printf("send_confirmation:sende ohne socket.\n");)
  return;
  }
header->size=size;
swap_falls_noetig(header, sizeof(msgheader)/sizeof(int));
swap_falls_noetig(confbuf, size);

da=send(ns, header, sizeof(msgheader), 0);

DV(D_COMM, printf("sent msgheader: %d bytes of %d.\n", da, sizeof(msgheader));)
DV(D_COMM, index=0;)
bufptr=(char*)confbuf;
restlen=size*sizeof(int);
while (restlen)
  {
  da=send(ns, bufptr, (restlen>MAXBYTES?MAXBYTES:restlen), 0);
  if (da>0)
    {
    bufptr+=da;
    restlen-=da;
    DV(D_COMM, index+=da; printf("sent body: %d bytes (%d of %d).\n", da,
       index, size*sizeof(int));)
    }
  else if (errno==EWOULDBLOCK)
    {
    DV(D_COMM, printf("send body would block.\n");)
    }
  else
    {
    printf("Fataler Fehler: Kann keine Confirmation absetzen\n");
    exit(errno);
    }
  }
}

/*****************************************************************************/

free_msg(msg)
int *msg;
{
T(free_msg)
if (msg) free((char*)msg);
}

/*****************************************************************************/
/*****************************************************************************/

