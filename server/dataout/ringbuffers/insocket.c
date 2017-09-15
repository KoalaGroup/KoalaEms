/******************************************************************************
*                                                                             *
* insocket.c                                                                  *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 28.02.94                                                                    *
*                                                                             *
******************************************************************************/

#define SSM
#define READOUT_PRIOR 10
#define HANDLERCOMNAME "handlercom"

#include <stdio.h>
#include <module.h>
#include <errno.h>
#include <types.h>

#include <in.h>
#include <socket.h>

#ifdef SSM
#include <ssm.h>
#endif

#include "handlers.h"

/*****************************************************************************/

static int s,evid,fertig,sockerror,*buffer;
static mod_exec *head;
static VOLATILE int *ip;

/*****************************************************************************/

void gib_weiter(buf,size)
int *buf,size;
{
    int restlen,da;
    char *bufptr;
    bufptr=(char*)buf;
    restlen=size*sizeof(int);
    while(restlen){
        da=send(s,bufptr,restlen,0);
        if(da>0){
            bufptr+=da;
            restlen-=da;
        }else if(da==-1){
            sockerror=errno;
            fertig=3;
            return;
        }
    }
}

/*****************************************************************************/

cleanup(){
    if(evid!=-1)_ev_unlink(evid);
    if(head!=(mod_exec*)-1)munlink(head);
    if(s!=-1)close(s);
    exit(0);
}

/*****************************************************************************/

void liesringbuffer()
{
    register int len;
    if(!(*ip)){
#ifndef NOSMARTPOLL
        tsleep(1);
#endif
        return;
    }
    len= *(ip+1);
    if(len>-1){
        gib_weiter(ip+1,len+1);
        *ip=0;
        ip+=len+2;
    }else{
        *ip=0;
        if(len==-2)fertig=2;
        else ip=buffer;
    }
}

main(argc,argv)
int argc;char **argv;
{
char *handlercomname;
int was;
int i;

struct sockaddr_in sa;
int host, port;

printf("insocket: argc=%d\n", argc);
for (i=0; i<argc; i++) printf("[%d]: >%s<\n", i, argv[i]);
if (argc<4) exit(0);
printf("insocket: T 1\n");
sockerror=0;

s=socket(AF_INET,SOCK_STREAM,0);
if (s==-1) sockerror=errno;
printf("insocket: socket=%d\n", s);
#ifndef OSK
if (fcntl(s, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("insocket: fcntl(s, FD_CLOEXEC): %s\n", strerror(errno));
  }
#endif
sa.sin_family=AF_INET;
if (sscanf(argv[1], "%d", &host)!=1) sockerror=E_BNAM;
sa.sin_addr.s_addr=htonl(host);
if (sscanf(argv[2], "%d", &port)!=1) sockerror=E_BNAM;
sa.sin_port=htons(port);
printf("insocket: vor connect\n");
if (connect(s, &sa, sizeof(sa))==-1) sockerror=errno;
printf("insocket: T2 sockerror=%d\n", sockerror);
if (argc>=6)
  {
  int bufsize;
  if (sscanf(argv[argc-2], "%x", &buffer)!=1) sockerror=E_BNAM;
  if (sscanf(argv[argc-1], "%x", &bufsize)!=1)sockerror=E_BNAM;
#ifdef SSM
  if (!sockerror) permit(buffer, bufsize, ssm_RW);
#endif
  head=(mod_exec*)-1;
  }
else
  {
  head=(mod_exec*)modlink(argv[3], mktypelang(MT_DATA, ML_ANY));
  if (head==(mod_exec*)-1) sockerror=errno;
  buffer=(int*)((char*)head+head->_mexec);
  }
printf("insocket: T3 sockerror=%d; head=%x\n", sockerror, head);

evid=_ev_link(argv[3]);
if (evid==-1) exit(errno);
printf("insocket: T4\n");
if (argc>6)
  handlercomname=argv[6];
else if (argc==5)
  handlercomname=argv[4];
else
  handlercomname=HANDLERCOMNAME;

intercept(cleanup);
fertig=1;
printf("insocket: T5\n");
for(;;)
  {
  if (fertig)
    {
    printf("insocket: fertig\n");
    was=_ev_wait(evid,HANDLER_MINVAL,HANDLER_MAXVAL);
    _ev_set(evid,HANDLER_IDLE,0);
    }
  else
    {
    int i;
    printf("insocket: nicht fertig\n");
    for (i=0; ((i<READOUT_PRIOR) && (!fertig)); i++) liesringbuffer();
    was=_ev_read(evid);
    _ev_set(evid,HANDLER_IDLE,0);
    }
  printf("insocket: T6; was=%x\n", was);
  if (was==HANDLER_START)
    {
    printf("insocket: HANDLER_START\n");
    if (!sockerror)
      {
      fertig=0;
      ip=buffer;
      }
    }
  else if (was==HANDLER_QUIT)
    {
    printf("insocket: HANDLER_QUIT\n");
    while (!(sockerror||fertig)) liesringbuffer();
    cleanup();
    }
  else if (was==HANDLER_GETSTAT)
    {
    mod_exec *hmod;
    int *buf;
    printf("insocket: HANDLER_GETSTAT\n");
    hmod=(mod_exec*)modlink(handlercomname, mktypelang(MT_DATA, ML_ANY));
    if (hmod==(mod_exec*)-1) exit(errno);
    buf=(int*)((char*)hmod+hmod->_mexec);
    buf[1]=3;
    buf[2]=sockerror;
    buf[3]=fertig;
    buf[4]= -1;
    buf[0]=0;
    munlink(hmod);
    }
  else if (was==HANDLER_WRITE)
    {
    mod_exec *hmod;
    int *buf;
    printf("insocket: HANDLER_WRITE\n");
    hmod=(mod_exec*)modlink(handlercomname, mktypelang(MT_DATA, ML_ANY));
    buf=(int*)((char*)hmod+hmod->_mexec);
    if (!sockerror) gib_weiter(buf+2,buf[1]);
    buf[0]=0;
    munlink(hmod);
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/
