/*
 * dataout/stream/file/stream.c
 * created before 08.12.96
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: stream.c,v 1.6 2011/04/06 20:30:22 wuestner Exp $";

#include <errno.h>
#include <fcntl.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include <rcs_ids.h>

#include "../../../objects/domain/dom_dataout.h"
#include "../../../objects/pi/readout.h"
#include "../../dataout.h"

int *next_databuf;
int buffer_free;

#ifndef EVENT_BUFBEG
static int _databuf[2*(event_max+1)+1],*databuf;
#else
static int *databuf=(int*)EVENT_BUFBEG;
#endif

static int datalen,welcher;

extern ems_u32* outptr;

static int fd;
static enum{closed, opening, ready, wrpending}connstate;

RCS_REGISTER(cvsid, "dataout/stream/file")

/*****************************************************************************/
int printuse_output(FILE* outfilepath)
{
return 0;
}
/*****************************************************************************/
errcode dataout_init(char *arg){
  T(dataout_init)
#ifndef EVENT_BUFBEG
#ifdef OSK
  databuf=(int*)(((int)_databuf+3)&0xfffffff0); /*4-Byte-Alignment erzwingen*/
#else
  databuf=_databuf;
#endif
#endif
  datalen=0;
  fd= -1;
  connstate=closed;
  buffer_free=0;
  return(OK);
}

errcode dataout_done(){
  T(dataout_done)
  if(fd>=0)close(fd);
  fd= -1;
  return(OK);
}

static int try_to_open(){
printf("opening\n");
  fd=
#ifdef OSK
    creat(dataout[0].addr.driver.path,3);
#else
    open(dataout[0].addr.driver.path,O_WRONLY|O_CREAT|O_NONBLOCK,0644);
#endif
  if(fd<0){
    printf("open error (%d/%d)\n",fd,errno);
    if((errno==EINTR)||(errno==ENXIO)){
      connstate=opening;
    }else{
      *outptr++=errno;
      return(-1);
    }
  }else{
    if (fcntl(fd, F_SETFD, FD_CLOEXEC)<0)
      {
      printf("stream.c: fcntl(fd, FD_CLOEXEC): %s\n", strerror(errno));
      }
    connstate=ready;
  }
  return(0);
}

static int try_to_write(){
  int res;
  if((res=write(fd, next_databuf-1, (datalen+1)*4))!=(datalen+1)*4){
    if(res<0){
      if((errno==EINTR)||
#ifdef ultrix
	 (errno==EWOULDBLOCK)
#else
	 (errno==EAGAIN)
#endif
	 ){
	connstate=wrpending;
	buffer_free=0;
	return(0);
      }
      printf("write error (%d/%d)\n",res,errno);
      if(errno==EPIPE){
	printf("broken pipe\n");
	if(dataout[0].wieviel)return(-1);
      }
    }else{
      printf("unexpected write result (%d/%d)\n",res,errno);
      return(-1);
    }
  }
  if(welcher){
    next_databuf=databuf+1+event_max;
    welcher=0;
  }else{
    next_databuf=databuf+1;
    welcher=1;
  }
  connstate=ready;
  buffer_free=1;
  return(0);
}

errcode start_dataout(){
  T(start_dataout)
  switch(connstate){
    case closed:
      return(Err_NoDomain);
    case wrpending:
      close(fd);
    case opening:
      if(try_to_open()<0)return(Err_System);
    case ready:
      datalen=0;
      welcher=1;
      next_databuf=databuf+1;
      if(connstate==ready)buffer_free=1;
      else{
	if(!dataout[0].wieviel){
	  printf("prio=0, no conn\n");
	  return(Err_System);
	}
	buffer_free=0;
      }
      break;
    default:
      printf("internal error (start_dataout): connstate=%d\n",connstate);
      return(Err_Program);
  }
  return(OK);
}

errcode stop_dataout()
{
  T(stop_dataout)
  return(OK);
}

void flush_databuf(p)
int *p;
{
  T(flush_databuf)
  if(connstate!=ready){
    printf("internal error (flush_databuf): connstate=%d\n",connstate);
    fatal_readout_error();
    return;
  }
  datalen=p-next_databuf;
  *(next_databuf-1)=datalen;
  if(try_to_write()<0){
    printf("flush error\n");
    fatal_readout_error();
    return;
  }
}

errcode get_last_databuf(){
  register int len=datalen;
  register int *ptr=(welcher?databuf+1+event_max:databuf+1);

  T(get_last_databuf)
  if(len){
    while (len--)*outptr++= *ptr++;
    return(OK);
  }else return(Err_NoDomain);
}

void schau_nach_speicher()
{
  T(schau_nach_speicher)
  switch(connstate){
    case opening:
      if(try_to_open()<0){
	printf("open error\n");
	fatal_readout_error();
	return;
      }
      break;
    case wrpending:
      if(try_to_write()<0){
	printf("flush error\n");
	fatal_readout_error();
	return;
      }
      break;
    case ready:
      break;
    default:
      printf("internal error (schau_nach_speicher): connstate=%d\n",connstate);
      fatal_readout_error();
      return;
  }
  if(connstate==ready)buffer_free=1;
}

errcode insert_dataout(i)
int i;
{
  T(insert_dataout)
  if(i!=0)return(Err_IllDomain);
  if(dataout[0].bufftyp==InOut_Stream){
    switch(dataout[i].addrtyp){
      case Addr_File:
	if(try_to_open()<0)return(Err_System);
	break;
      default:
	return(Err_AddrTypNotImpl);
    }
  }else return(Err_BufTypNotImpl);
  return(OK);
}

errcode remove_dataout(i)
int i;
{
  T(remove_dataout)
  if(i!=0)return(Err_ArgRange);
  if(dataout[0].bufftyp==InOut_Stream){
    switch(dataout[i].addrtyp){
      case Addr_File:
	if(fd>=0){
	  close(fd);
	  fd= -1;
	}
	break;
      default:
	return(Err_AddrTypNotImpl);
    }
  }else return(Err_BufTypNotImpl);
  return(OK);
}
