/*
 * commu/socket/socketcomm.c
 * created long before 22.08.96
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: socketcomm.c,v 1.30 2016/11/26 01:18:16 wuestner Exp $";

#include <sconf.h>
#include <config.h>
#if defined(__unix__) || defined(unix__) || defined(Lynx) || defined(__Lynx__)
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef COMMU_LOCALSOCKET
#include <sys/un.h>
#endif
#else
#ifdef OSK
#include <types.h>
#include <in.h>
#include <socket.h>
#include <netdb.h>
#include <sgstat.h>
#endif
#endif
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#include <errorcodes.h>
#include <swap.h>
#include <debug.h>

#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif

#include <emssocktypes.h>
#include "../commu.h"
#include "socketcomm.h"
#include "../../main/scheduler.h" /* for breakflag */
#include <rcs_ids.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/*
 * #ifdef NEED_INDEX
 * char *index(const char*, int);
 * #endif
 */

RCS_REGISTER(cvsid, "commu/socket")

/*****************************************************************************/

static int s, ns;
static struct sockaddr *sock;

#ifdef COMMU_LOCALSOCKET
static int wehavetoswap;
#endif

#ifdef SELECTCOMM
#define noblock(s)
#define block(s)
#else /* SELECTCOMM */
static void noblock(int s)
{
#ifdef OSK
  struct sgbuf optbuf;
#endif /* OSK */
T(socketcomm.c::noblock)
  if(s<0){
    printf("noblock on nosocket!\n");
    return;
  }

#if defined(__unix) || defined(__unix__) || defined(Lynx)
  fcntl(s, F_SETFL, O_NDELAY);
#else /* unix || Lynx */
#ifdef OSK
  _gs_opt(s, &optbuf);
  optbuf.sg_noblock=1;
  _ss_opt(s, &optbuf);
#else /* OSK */
  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#endif /* OSK */
#endif /* unix || Lynx */
}

static void block(int s)
{
#ifdef OSK
  struct sgbuf optbuf;
#endif
T(socketcomm.c::block)

  if(s<0){
    printf("block on nosocket!\n");
    return;
  }

#if defined(__unix) || defined(__unix__) || defined(Lynx)
  fcntl(s, F_SETFL, 0);
#else
#ifdef OSK
  _gs_opt(s, &optbuf);
  optbuf.sg_noblock=0;
  _ss_opt(s, &optbuf);
#else
  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#endif
#endif
}
#endif /* SELECTCOMM */
/*****************************************************************************/

#ifdef COMMU_ACCESSCONTROL
static struct in_addr *allowed_clients;
static int clnum;

static unsigned int gethostnumber(arg)
char *arg;
{
  int number;
  struct hostent *he;
T(socketcomm.c::gethostnumber)
  if((number=inet_addr(arg))!=-1)return(number);
  if(he=gethostbyname(arg))return(*(int*)((he)->h_addr_list[0]));
  printf("Host %s ist unbekannt\n",arg);
  return(-1);
}
/*****************************************************************************/

static int validate_client(adr)
struct sockaddr *adr;
{
  int i;
T(socketcomm.c::validate_client)
  if(
#ifdef COMMU_LOCALSOCKET
     (adr->sa_family!=AF_INET)||
#endif
     (!clnum))return(1);
  for(i=0;i<clnum;i++){
    if(((struct sockaddr_in*)adr)->sin_addr.s_addr==allowed_clients[i].s_addr){
      D(D_COMM, printf("connection from %s accepted\n",
		       inet_ntoa(((struct sockaddr_in*)adr)->sin_addr));)
      return(1);
    }
  }
  printf("connection from %s refused\n",
	 inet_ntoa(((struct sockaddr_in*)adr)->sin_addr));
  return(0);
}
#endif

/*****************************************************************************/
int printuse_port(FILE* outfilepath)
{
fprintf(outfilepath,
"  [:usocket=<unixsocket>][:clients=<client{,client}>][:port#<portnum>]\n"
"  | <portnum>\n"
"    defaults: portnum=%d\n", DEFPORT);
return 1;
}
/*****************************************************************************/

errcode _init_comm(char* port)
{
  int p; long lp;
#ifdef COMMU_LOCALSOCKET
  char *ustr;
#endif
  size_t sasize;
  unsigned int optval;
  struct linger ling;

  T(socketcomm.c::_init_comm)
  p=DEFPORT;
#ifdef COMMU_LOCALSOCKET
  ustr=(char*)0;
#endif
  if(port){
    if(strchr(port,':')){
#ifdef COMMU_LOCALSOCKET
      if(cgetstr(port, "usocket", &ustr)<0)
#endif
      {
#ifdef COMMU_ACCESSCONTROL
	char *clstr;
	if(cgetstr(port, "clients", &clstr)<0){
	  allowed_clients=(struct in_addr*)0;
	  clnum=0;
	}else{
	  char *help;
	  int i;
	  clnum=1;
	  help=clstr;
	  while(help=strchr(help,',')){
	    clnum++;
	    help++;
	  }
	  allowed_clients=(struct in_addr*)calloc(clnum,sizeof(struct in_addr));
	  if(!allowed_clients)return(Err_NoMem);
	  help=clstr;
	  for(i=0;i<clnum;i++){
	    char *end;
	    end=strchr(help,',');
	    if(end)*end='\0';
	    if((allowed_clients[i].s_addr=gethostnumber(help))==-1){
	      free(allowed_clients);
	      allowed_clients=(struct in_addr*)0;
	      clnum=0;
	      return(Err_ArgRange);
	    }
	    D(D_COMM, printf("adding client %s\n",inet_ntoa(allowed_clients[i]));)
	      if(end)*end=',';
	    help=end+1;
	  }
	}
#endif
	cgetnum(port, "port", &lp); /* XXX error check not possible */
	p=lp;
      }
    }else{ /* old arg style, for backward compatibility */
      if(sscanf(port, "%d", &p)!=1){
	printf("can't decode port number! (%s)\n", port);
	return(Err_ArgRange);
      }
    }
  }
#ifdef COMMU_LOCALSOCKET
  if(ustr){
    struct sockaddr_un *sun;
    D(D_COMM, printf("using socket %s\n", ustr);)
    sasize=sizeof(struct sockaddr_un);
    sock=(struct sockaddr*)(sun=(struct sockaddr_un*)malloc(sasize));
    sun->sun_family=AF_UNIX;
    strcpy(sun->sun_path,ustr);
    wehavetoswap=0;
  }else
#endif
  {
    struct sockaddr_in *sockin;
    D(D_COMM, printf("using socket on port %d\n", p);)
#ifdef COMMU_ACCESSCONTROL
    D(D_COMM, if(!clnum)printf("no access control!\n");)
#endif
    sasize=sizeof(struct sockaddr_in);
    sock=(struct sockaddr*)(sockin=(struct sockaddr_in*)malloc(sasize));
    sockin->sin_family=AF_INET;
    sockin->sin_port=htons(p);
    sockin->sin_addr.s_addr=INADDR_ANY;
#ifdef COMMU_LOCALSOCKET
    wehavetoswap=1;
#endif
  }

  if((s=socket(sock->sa_family, SOCK_STREAM, 0))<0){
    printf("can't open socket: %s\n", strerror(errno));
    return(Err_System);
  }
  if (fcntl(s, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("socketcomm.c: fcntl(s, FD_CLOEXEC): %s\n", strerror(errno));
    }
  
  optval=1;
  /*printf("setze SO_REUSEADDR\n");*/
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0)
    {
    perror("setsockopt(SO_REUSEADDR)");
    }
  if(bind(s, sock, sasize)<0){
    printf("error in bind: %s\n", strerror(errno));
    return(Err_System);
  }

  ling.l_onoff=1;
  ling.l_linger=3;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, &ling, sizeof(struct linger))<0)
    printf("_init_comm: setsockopt(LINGER): %s\n", strerror(errno));

  noblock(s);

  listen(s, 1);
  ns= -1;
  return(OK);
}
/*****************************************************************************/

errcode _done_comm(){
T(socketcomm.c::_done_comm);
#ifdef COMMU_ACCESSCONTROL
  if(allowed_clients)free(allowed_clients);
  allowed_clients=(struct in_addr*)0;
  clnum=0;
#endif
  if(ns>=0)close(ns);
  if(s>=0){
    close(s);
#ifdef COMMU_LOCALSOCKET
    if(sock->sa_family==AF_UNIX)
      unlink(((struct sockaddr_un*)sock)->sun_path);
#endif
  }
  if(sock)free(sock);
  return(OK);
}

/*****************************************************************************/

int getlistenpath(void)
{
return s;
}

int getcommandpath(void)
{
return ns;
}

/*****************************************************************************/

int open_connection(){
  struct sockaddr sockin;
  ems_socklen_t len;
  int optval;

  T(socketcomm.c::open_connection)
  if(ns>=0){
    printf("Fehler: open while connected\n");
    return(-1);
  }
  block(s);
  while(ns<0){
    len=sizeof(struct sockaddr);
    if((ns=accept(s,&sockin,&len))<0){
      if
#ifdef OSK
	(errno==ECONNABORTED)
#else
	(errno==EINTR)
#endif
      {
	if(breakflag)
	  return(0);
	else continue;
      }
      printf("error in accept: %s\n", strerror(errno));
      return(-1);
    }
    if (fcntl(ns, F_SETFD, FD_CLOEXEC)<0)
      {
      printf("socketcomm.c: fcntl(ns, FD_CLOEXEC): %s\n", strerror(errno));
      }

#ifdef COMMU_ACCESSCONTROL
    if(!validate_client(&sockin)){
      close_connection();
      continue;
    }
#endif
#ifndef OSK
    if(sockin.sa_family==AF_INET){
      optval=1;
      if(setsockopt(ns,IPPROTO_TCP,TCP_NODELAY,(char*)&optval,
		    sizeof(optval))<0){
	printf("setsockopt(NODELAY): %s\n", strerror(errno));
      }
    }
#endif
  }
  noblock(ns);
  noblock(s);
  return(1);
}
/*****************************************************************************/

int poll_connection(){
  struct sockaddr sockin;
  ems_socklen_t len;
  int optval;

  T(socketcomm.c::poll_connection)
  if(ns>=0){
    printf("Fehler: poll while connected\n");
    return(-1);
  }
  len=sizeof(struct sockaddr);
  ns=accept(s, &sockin, &len);
  if(ns<0){
    if(errno==EWOULDBLOCK)return(0);
    printf("accept failed: %s\n", strerror(errno));
    return(-1);
  }
  if (fcntl(ns, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("socketcomm.c: fcntl(ns, FD_CLOEXEC): %s\n", strerror(errno));
    }

#ifdef COMMU_ACCESSCONTROL
  if(!validate_client(&sockin)){
    close_connection();
    return(0);
  }
#endif
#ifndef OSK
  if(sockin.sa_family==AF_INET){
    optval=1;
    if(setsockopt(ns,IPPROTO_TCP,TCP_NODELAY,(char*)&optval,
		  sizeof(optval))<0){
      printf("setsockopt(NODELAY): %s\n", strerror(errno));
    }
  }
#endif
  noblock(ns);
  return(1);
}

/*****************************************************************************/

void close_connection(){
  T(socketcomm.c::close_connection)
  if(ns<0){
    printf("Fehler: socketcomm.c::close_connection() called while not connected\n");
    return;
  }
  close(ns);
  ns= -1;
}
/*****************************************************************************/

int recv_data(ems_u32* buf, unsigned int len)
{
  int da;
  unsigned int restlen;
  char *bufptr;

  T(socketcomm.c::recv_data)
  DV(D_COMM,printf("lese %d Worte\n",len);)
  if(ns<0){
    printf("Fehler: recv while not connected\n");
    return(-1);
  }
  block(ns);
  restlen=len*4;
  bufptr=(char*)buf;
  while(restlen){
    /*printf("recv(ns=%d, bufptr=%p, restlen=%d)\n", ns,bufptr,restlen);*/
    da=recv(ns, bufptr, restlen, 0);
    if(da>0){
      bufptr+=da;
      restlen-=da;
    }else{
      if((da<0)&&
#ifdef OSK
	 (errno==E_READ)
#else
	 (errno==EINTR)
#endif
	 ){
	if(breakflag&&(restlen==len*4))return(0);
	continue;
      }
      printf("recv: %d, errno=%s\n",da, strerror(errno));
      return(-1);
    }
  }
#ifdef COMMU_LOCALSOCKET
  if(wehavetoswap)
#endif
    swap_falls_noetig(buf, len);
  noblock(ns);
  DV(D_COMM,printf("gelesen\n");)
  return(len);
}
/*****************************************************************************/

int poll_data(ems_u32* buf, unsigned int len)
{
  int da;
  unsigned int restlen;
  char *bufptr;

  T(socketcomm.c::poll_data)
  DV(D_COMM,printf("lese %d Worte\n",len);)
  if(ns<0){
      printf("Fehler: poll while not connected\n");
      return(-1);
  }
  restlen=len*4;
  bufptr=(char*)buf;
  da=recv(ns, bufptr, restlen, 0);
  if(da<=0){
    if(errno==EWOULDBLOCK)return(0);
    printf("recv: %d, errno=%s\n",da, strerror(errno));
    return(-1);
  }
  bufptr+=da;
  restlen-=da;
  block(ns);
  while(restlen){
    da=recv(ns,bufptr,restlen,0);
    if(da>0){
      bufptr+=da;
      restlen-=da;
    }else{
      if((da<0)&&
#ifdef OSK
	 (errno==E_READ)
#else
	 (errno==EINTR)
#endif
	 ){
	if(breakflag&&(restlen==len*4))return(0);
	continue;
      }
      printf("recv: %d, errno=%s\n",da, strerror(errno));
      return(-1);
    }
  }
#ifdef COMMU_LOCALSOCKET
  if(wehavetoswap)
#endif
    swap_falls_noetig(buf,len);
  noblock(ns);
  DV(D_COMM,printf("gelesen\n");)
  return(len);
}
/*****************************************************************************/

int send_data(ems_u32* buf, unsigned int len)
{
  int weg;
  unsigned int restlen;
  char *bufptr;

  T(socketcomm.c::send_data)
  /*printf("send_data(buf=%p, len=%d, ns=%d)\n", buf, len, ns);*/
  DV(D_COMM,printf("schreibe %d Worte\n",len);)
  if (len<=0)
    {
    printf("send_data: len<0 (==%d)\n", len);
    return 0;
    }
  if(ns<0){
      printf("Fehler: send while not connected\n");
      return(-1);
  }
  block(ns);
#ifdef COMMU_LOCALSOCKET
  if(wehavetoswap)
#endif
    swap_falls_noetig(buf,len);
  restlen=len*4;
  bufptr=(char*)buf;
  while(restlen){
    weg=send(ns, bufptr, restlen, 0);
    if(weg>0){
      bufptr+=weg;
      restlen-=weg;
    }else{
      if((weg<0)&&
#ifdef OSK
	 (errno==E_WRITE)
#else
	 (errno==EINTR)
#endif
	 )continue;
      printf("send_data:send: %d, errno=%s\n",weg, strerror(errno));
      printf("send_data: buf=%p len=%u restlen=%u socket=%d",
        buf, len, restlen, ns);
      return(-1);
    }
  }
  noblock(ns);
  DV(D_COMM,printf("geschrieben\n");)
  return(0);
}
/*****************************************************************************/
/*****************************************************************************/
