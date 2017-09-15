/*
 * $ZEL: infile.c,v 1.5 2003/10/18 19:25:19 wuestner Exp $
 */

#define READOUT_PRIOR 10
#define HANDLERCOMNAME "handlercom"

#include <stdio.h>
#include <module.h>
#include <errno.h>
#include <types.h>
#include <modes.h>
#include <strings.h>

#include "handlers.h"

static int f,evid,fileerror,fertig,*buffer;
static mod_exec *head;
static VOLATILE int *ip;

void gib_weiter(buf,size)
int *buf,size;
{
    if(write(f,buf,size*sizeof(int))==-1){
	fileerror=errno;
	fertig=3;
	return;
    }
}

cleanup()
{
if(evid!=-1)_ev_unlink(evid);
if(head!=(mod_exec*)-1)munlink(head);
if(f!=-1)close(f);
exit(0);
}

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
  int was,generation;

  if(argc<3)exit(0);

  fileerror=0;
  generation=1;

  f=creat(argv[1],3);
  if(f==-1)fileerror=errno;

  head=(mod_exec*)modlink(argv[2],mktypelang(MT_DATA,ML_ANY));
  if(head==(mod_exec*)-1)fileerror=errno;
  buffer=(int*)((char*)head+head->_mexec);

  evid=_ev_link(argv[2]);
  if(evid==-1)exit(errno);

  if(argc>3)handlercomname=argv[3];
  else handlercomname=HANDLERCOMNAME;

  intercept(cleanup);
  fertig=1;
  for(;;)
  {
    if(fertig)
    {
      was=_ev_wait(evid,HANDLER_MINVAL,HANDLER_MAXVAL);
      _ev_set(evid,HANDLER_IDLE,0);
    }else{
      int i;
      for(i=0;((i<READOUT_PRIOR)&&(!fertig));i++)liesringbuffer();
      was=_ev_read(evid);
      _ev_set(evid,HANDLER_IDLE,0);
    }
    switch(was){
      case HANDLER_START:
	if(!fileerror){
	  fertig=0;
	  ip=buffer;
	}
	break;
      case HANDLER_QUIT:
	while (!(fileerror||fertig))liesringbuffer();
	cleanup();
	break;
      case HANDLER_GETSTAT:{
	mod_exec *hmod;
	int *buf;
	hmod=(mod_exec*)modlink(handlercomname,mktypelang(MT_DATA,ML_ANY));
	if(hmod==(mod_exec*)-1)exit(errno);
	buf=(int*)((char*)hmod+hmod->_mexec);
	buf[1]=3;
	buf[2]=fileerror;
	buf[3]=fertig;
	buf[4]= -1;
	buf[0]=0;
	munlink(hmod);
	}
	break;
      case HANDLER_WRITE:{
	mod_exec *hmod;
	int *buf;
	hmod=(mod_exec*)modlink(handlercomname,mktypelang(MT_DATA,ML_ANY));
	if(hmod==(mod_exec*)-1){
	  fileerror=errno;
	  break;
	}
	buf=(int*)((char*)hmod+hmod->_mexec);
	if(!fileerror)write(f,buf+2,buf[1]*sizeof(int));
	buf[0]=0;
	munlink(hmod);
	}
	break;
      default:
	if((was>-MAX_WIND)&&(was<MAX_WIND)&&(!fileerror)){
	  char *pos,dirname[256],*oldname,newname[256];
	  int p;
	  if(pos=rindex(argv[1],'/')){
	    strncpy(dirname,argv[1],pos-argv[1]);
	    dirname[pos-argv[1]]='\0';
	    oldname=pos+1;
	  }else{
	    strcpy(dirname,".");
	    oldname=argv[1];
	  }
	  if((p=open(dirname,0x82))==-1){
	    fileerror=errno;
	    break;
	  }
	  snprintf(newname, 256,"%s.%d",oldname,generation++);
	  if(_setstat(p,66,oldname,newname)==-1)fileerror=errno;
	  close(p);
	  if(!fileerror){
	    close(f);
	    f=creat(argv[1],3);
	    if(f==-1)fileerror=errno;
	  }
	}
	break;
    }
  }
}

