/* Gateway von TCP/IP auf OS/9-Pipe */
/* von "gwd.c" abgeleitet, verwaltet eine Verbindung */
static const char* cvsid __attribute__((unused))=
    "$ZEL: emsnetgate.c,v 1.4 2011/04/06 20:30:21 wuestner Exp $";

#include <stdio.h>
#include <errno.h>

#include <ems_err.h>
#include <msg.h>
#include <intmsgtypes.h>
#include <xdrstring.h>
#include <rcs_ids.h>

#define SIG 100

#define GWBUFSIZ 512
static int buffer[GWBUFSIZ];

static msgheader h;
static int sock,sig,ppid;
static int lock,curxid;

RCS_REGISTER(cvsid, "")

static void process_request(){
  int da,restlen;
  char *bufptr;

  da=_gs_rdy(sock);
  if(da<=0)return;

  /* Message von Socket in Ausgabepipe (stdout) */
  bufptr=(char*)&h;
  restlen=sizeof(msgheader);
  while(restlen){
    da=recv(sock,bufptr,restlen,0);
    if(da>0){
      bufptr+=da;
      restlen-=da;
    }
  }
  write(1,&h,sizeof(msgheader));

  kill(ppid,sig);

  restlen=h.size*sizeof(int);
  while(restlen){
    da=recv(sock,buffer,restlen<GWBUFSIZ?restlen:GWBUFSIZ,0);
    if(da>0){
      restlen-=da;
      write(1,buffer,da);
    }
  }curxid=h.transid; /* vorlaeufig keine weiteren Requests */
  lock=1;
    _ss_ssig(sock,SIG);
}

static void xsend(s,buf,len)
int s;
char *buf;
int len;
{
  int da;
  while(len){
    da=send(s,buf,len,0);
    if(da>0){
      buf+=da;
      len-=da;
    }
  }
}

static int process_confirmation(){
  int da, restlen;
  char *bufptr;

/*  if(_gs_rdy(0)<=0)return(0);*/
  while(_gs_rdy(0)>0){
    /* Message von Eingabepipe (stdin) auf socket */
    bufptr=(char*)&h;
    restlen=sizeof(msgheader);
    while(restlen){
      da=_gs_rdy(0);
      if(da<0){
	tsleep(1);
	continue;
      }
      da=read(0,bufptr,da<restlen?da:restlen);
      if(da>0){
	bufptr+=da;
	restlen-=da;
      }
    }
    xsend(sock,&h,sizeof(msgheader));
    restlen=h.size*sizeof(int);
    while(restlen){
      da=_gs_rdy(0);
      if(da<0){
	tsleep(1);
	continue;
      }
      if(da>GWBUFSIZ)da=GWBUFSIZ;
      da=read(0,buffer,da<restlen?da:restlen);
      if(da>0){
	restlen-=da;
	xsend(sock,buffer,da);
      }
    }
    if(!(h.flags&Flag_Unsolicited)){
      if(h.transid==curxid)
	lock=0; /* neue Requests zugelassen */
      else
	_errmsg(0,"Unerwartete xid in Confirmation\n");
    }
    if((h.flags&Flag_Intmsg)&&(h.type.intmsgtype==Intmsg_Close))return(1); /* Ende */
  }
    _ss_ssig(0,SIG);
  return(0);
}

ignore(){} /* wir fliegen aus dem sleep raus */

main(argc,argv)
int argc;char **argv;
{
  int s,*buffer;
  int closing;
  int da,restlen;
  char *bufptr;

  if(argc<3)exit(0);
  if(sscanf(argv[1],"%d",&s)!=1)exit(0);
  if(sscanf(argv[2],"%d",&sig)!=1)exit(0);
  ppid=getppid();

  intercept(ignore);

  sock=accept(s,1);

  bufptr=(char*)&h;
  restlen=sizeof(msgheader);
  while(restlen){
    da=recv(sock,bufptr,restlen,0);
    if(da>0){
      bufptr+=da;
      restlen-=da;
    }
  }

  if(!((h.flags&Flag_Intmsg)&&(h.type.intmsgtype==Intmsg_Open)))
    exit(_errmsg(0,"Fatal - erwarte Intmsg_Open!\n"));

  if(h.size){
    buffer=(int*)calloc(h.size,sizeof(int));
    bufptr=(char*)buffer;
    restlen=h.size*sizeof(int);
    while(restlen){
      da=recv(sock,buffer,restlen,0);
      if(da>0){
	bufptr+=da;
	restlen-=da;
      }
    }
  }else buffer=(int*)0;
  
  sigmask(1);
  _ss_ssig(0,SIG);
  _ss_ssig(sock,SIG);

  /* gebe Open-Message weiter */
  write(1,&h,sizeof(msgheader));
  if(h.size)write(1,buffer,h.size*sizeof(int));
  if(buffer)free(buffer);

  kill(ppid,sig);

  /* deren Confirmation wird in der Hauptschleife zurueckgereicht */
  /* bis dahin keine neuen Requests */
  curxid=h.transid;
  lock=1;

  do{
    sleep(0);
    sigmask(1);
    if(!lock)process_request();
    closing=process_confirmation(); /* closing = letzter Transfer */
    _ss_ssig(0,SIG);
    _ss_ssig(sock,SIG);
  }
  while(!closing);

  close(0);
  close(1);
  close(sock);
}
