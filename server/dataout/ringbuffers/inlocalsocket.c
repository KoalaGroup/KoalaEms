/******************************************************************************
*                                                                             *
* inlocalsocket.c                                                             *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 28.02.94                                                                    *
*                                                                             *
******************************************************************************/

#define READOUT_PRIOR 10
#define HANDLERCOMNAME "handlercom"

#include <stdio.h>
#include <module.h>
#include <errno.h>
#include <types.h>

#include <un.h>
#include <socket.h>

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

    struct sockaddr_un sa;
    char* sockname;

    if(argc<3)exit(0);

    sockerror=0;

    s=socket(AF_UNIX,SOCK_STREAM,0);
    if(s==-1)sockerror=errno;
    memset(&sa,0,sizeof(sa));
    sa.sun_family=AF_UNIX;
    strcpy(sa.sun_path,argv[1]);
    if(connect(s,&sa,sizeof(sa))==-1)sockerror=errno;

    head=(mod_exec*)modlink(argv[2],mktypelang(MT_DATA,ML_ANY));
    if(head==(mod_exec*)-1)sockerror=errno;
    buffer=(int*)((char*)head+head->_mexec);

    evid=_ev_link(argv[3]);
    if(evid==-1)return(errno);

    if(argc>3)handlercomname=argv[3];
    else handlercomname=HANDLERCOMNAME;

    intercept(cleanup);
    fertig=1;
    for(;;){
        if(fertig)
        {
            was=_ev_wait(evid,HANDLER_MINVAL,HANDLER_MAXVAL);
            _ev_set(evid,HANDLER_IDLE,0);
        }
        else
        {
            int i;
            for(i=0;((i<READOUT_PRIOR)&&(!fertig));i++)liesringbuffer();
            was=_ev_read(evid);
            _ev_set(evid,HANDLER_IDLE,0);
        }
        if(was==HANDLER_START){
            if(!sockerror){
                fertig=0;
                ip=buffer;
            }
        }else if(was==HANDLER_QUIT){
            while(!(sockerror||fertig))liesringbuffer();
            cleanup();
        }else if(was==HANDLER_GETSTAT){
            mod_exec *hmod;
            int *buf;
            hmod=(mod_exec*)modlink(handlercomname,mktypelang(MT_DATA,ML_ANY));
            if(hmod==(mod_exec*)-1)exit(errno);
            buf=(int*)((char*)hmod+hmod->_mexec);
            buf[1]=3;
            buf[2]=sockerror;
            buf[3]=fertig;
            buf[4]= -1;
            buf[0]=0;
            munlink(hmod);
        }else if(was==HANDLER_WRITE){
            mod_exec *hmod;
            int *buf;
            hmod=(mod_exec*)modlink("handlercom",mktypelang(MT_DATA,ML_ANY));
            buf=(int*)((char*)hmod+hmod->_mexec);
            if(!sockerror)gib_weiter(buf+2,buf[1]);
            buf[0]=0;
            munlink(hmod);
        }
    }
}

/*****************************************************************************/
/*****************************************************************************/

