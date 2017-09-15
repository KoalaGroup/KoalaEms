static const char* cvsid __attribute__((unused))=
    "$ZEL: socketcomm.c,v 1.4 2011/04/06 20:30:21 wuestner Exp $";

#include <sconf.h>

#include <types.h>
#include <in.h>
#include <socket.h>
#include <netdb.h>
#include <sgstat.h>
#include <errno.h>
#include <signal.h>

#include <msg.h>
#include <requesttypes.h>
#include <intmsgtypes.h>
#include <errorcodes.h>
#include <swap.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../../main/signals.h"

#define GATEWAY "emsnetgate"

static int s,pin,pout,sig,flag,gwpid;
extern msgheader header;
extern int connected;

static void setflag(sig,arg)
int sig;
ClientData arg;
{
  flag=1;
}

extern char **environ;
extern int os9forkc();

RCS_REGISTER(cvsid, "")

static int startgateway()
{
  char *argv[4];
  char sigstr[15],sockstr[15];
  ClientData arg;
  int p1,p2;

  arg.val=0;
  sig=install_signalhandler(setflag,arg);
  if(sig==-1)return(E_MEMFUL);
  flag=1;

  sprintf(sockstr,"%d",s);
  sprintf(sigstr,"%d",sig);
  argv[0]=GATEWAY;
  argv[1]=sockstr;
  argv[2]=sigstr;
  argv[3]=0;

  pout=open("/pipe",3);
  if(pout==-1)return(errno);
  p1=dup(0);
  close(0);
  dup(pout);
  pin=open("/pipe",3);
  if(pin==-1)return(errno);
  p2=dup(1);
  close(1);
  dup(pin);
  gwpid=os9exec(os9forkc,argv[0],argv,environ,0,0,s+1);
  close(0);
  dup(p1);
  close(p1);
  close(1);
  dup(p2);
  close(p2);
  if(gwpid==-1){
    return(errno);
  }
  return(0);

}

static void stopgateway(){
  unsigned int status;
  if(wait(&status)){
    gwpid= -1;
    close(pin);
    close(pout);
    remove_signalhandler(sig);
  }
}

errcode _init_comm(port)
char *port;
{
  int p,res;
  struct sockaddr_in sin;
  unsigned short optval;
  struct linger ling;

  T(_init_comm)
  p=DEFPORT;
  if((port)&&(sscanf(port,"%d",&p)!=1)){
    printf("Bloede Portnummer!(%s)\n",port);
    return(Err_ArgRange);
  }

  D(D_COMM,printf("using socket on port %d\n", p);)
  s=socket(AF_INET,SOCK_STREAM, 0);
  optval=1;
  setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
  ling.l_onoff=1;
  ling.l_linger=3;
  setsockopt(s,SOL_SOCKET,SO_LINGER,&ling,sizeof(struct linger));

  sin.sin_family=AF_INET;
  sin.sin_port=htons(p);
  sin.sin_addr.s_addr=INADDR_ANY;
  if(bind(s,&sin,sizeof(sin))==-1){
    printf("bind schlug fehl; errno=%d\n",errno);
    return(Err_System);
  }
  listen(s,1);
  connected=0;
  if(res=startgateway()){
    printf("kann Gateway nicht starten (%d)\n",res);
    return(Err_System);
  }
  return(OK);
}

errcode _done_comm(){
  if(gwpid!=-1){
    kill(gwpid,SIGKILL); /* kann nicht abgefangen werden */
    stopgateway();
  }
  return(OK);
}

Request get_request(reqbuf, siz)
int **reqbuf, *siz;
{
  struct sockaddr_in sin;
  int len, restlen, da, size, res;
  char *bufptr;
  int *msg;

  T(get_request)
  do{
    restlen=sizeof(msgheader);
    bufptr=(char*)&header;
    while(restlen){
      da=read(pin,bufptr,restlen);
      if(da>0){    /*falls Error, wird nicht gezaehlt*/
	DV(D_COMM,printf("%d Headerbytes angekommen\n",da);)
	bufptr+=da;
	restlen-=da;
      }
    }
    size=*siz=header.size;
    DV(D_COMM,printf("Request %d, Flags %d: es folgen %d Bytes!\n",
		     header.type.reqtype,header.flags,
		     size*sizeof(int));)
    if(size>0){
      msg= *reqbuf=(int*)calloc(size,sizeof(int));
      restlen=size*sizeof(int);
      bufptr=(char*)msg;
      while(restlen){
	da=read(pin,bufptr,restlen);
	if(da>0){
	  DV(D_COMM,printf("%d Bytes angekommen\n",da);)
	  bufptr+=da;
	  restlen-=da;
	}
      }
    }else msg= *reqbuf=(int*)0;
    if(header.flags&Flag_Intmsg){
      D(D_COMM, printf("Intmsg, sofort zurueck\n");)
      header.flags|=Flag_Confirmation;
      header.size=0;
      write(pout,&header,sizeof(msgheader));
      if(msg)free(msg);
      if(header.type.intmsgtype==Intmsg_Open)connected=1;
      else if(header.type.intmsgtype==Intmsg_Close){
	connected=0;
      stopgateway();
      startgateway();
      }
    }
  }while(header.flags&Flag_Intmsg);
  return(header.type.reqtype);
}

Request get_request_noblock(reqbuf, siz)
int **reqbuf, *siz;
{
  struct sockaddr_in sin;
  int len, restlen, da, size;
  char *bufptr;
  int *msg;

  T(get_request_noblock)
  if(!flag)return(Req_Nothing);
  if(_gs_rdy(pin)<1){
    flag=0;
    return(Req_Nothing);
  }
  restlen=sizeof(msgheader);
  bufptr=(char*)&header;
  while(restlen){
    da=read(pin, bufptr, restlen);
    if(da>0){    /*falls Error, wird nicht gezaehlt*/
      DV(D_COMM,printf("%d Headerbytes angekommen\n",da);)
	bufptr+=da;
      restlen-=da;
    }
  }
  size=*siz=header.size;
  DV(D_COMM, printf("Es folgen %d Bytes!\n",size*sizeof(int));)
  if(size>0){
    msg= *reqbuf=(int*)calloc(size,sizeof(int));
    restlen=size*sizeof(int);
    bufptr=(char*)msg;
    while(restlen){
      da=read(pin,bufptr,restlen);
      if(da>0){
	DV(D_COMM,printf("%d Bytes angekommen\n",da);)
	  bufptr+=da;
	restlen-=da;
      }
    }
  }else msg= *reqbuf=(int*)0;
  if(header.flags&Flag_Intmsg){
    header.flags|=Flag_Confirmation;
    header.size=0;
    write(pout,&header,sizeof(msgheader));
    if(msg)free(msg);
    if(header.type.intmsgtype==Intmsg_Open)connected=1;
    else if(header.type.intmsgtype==Intmsg_Close){
      connected=0;
      stopgateway();
      startgateway();
    }
    return(Req_Nothing);
  }
  return(header.type.reqtype);
}

void send_confirmation(header, confbuf, size)
msgheader *header;
int *confbuf;
int size;
{
  int res;

  T(send_confirmation)
  header->size=size;
  res=write(pout,(char*)header,sizeof(msgheader));
  DV(D_COMM,printf("sent msgheader: %d bytes of %d.\n",res,sizeof(msgheader));)
  res=write(pout,(char*)confbuf,size*sizeof(int));
  DV(D_COMM,printf("sent body: %d bytes of %d.\n",res,size*sizeof(int));)
}

void free_msg(msg)
int *msg;
{
  T(free_msg)
  if(msg)free((char*)msg);
}
