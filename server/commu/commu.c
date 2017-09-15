/*
 * commu/commu.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: commu.c,v 1.19 2011/04/06 20:30:21 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <msg.h>
#include <requesttypes.h>
#include <intmsgtypes.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "commu.h"
#include "../lowlevel/oscompat/oscompat.h"

#if defined(__unix__)
#include <stdlib.h>
#endif

#include "../lowlevel/oscompat/oscompat.h"
#ifdef LOWLEVELBUFFER
#include "../lowlevel/lowbuf/lowbuf.h"
#endif /* LOWLEVELBUFFER */

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static msgheader *header;
ems_u32* outbuf;

#ifdef OUT_BUFBEG
static mmapresc om;
#else /* OUT_BUFBEG */
#ifndef LOWLEVELBUFFER
static ems_u32 _outbuf[OUT_MAX+1];
#endif /* LOWLEVELBUFFER */
#endif /* OUT_BUFBEG */

#ifndef COMM_QUICKBUFSIZE
#define COMM_QUICKBUFSIZE 100
#endif
static unsigned int quickbuf[COMM_QUICKBUFSIZE];

connstates connstate;
const char* connstatename[]={
    "conn_no",
    "conn_opening",
    "conn_ok",
    "conn_closing",
};

RCS_REGISTER(cvsid, "commu")

/*****************************************************************************/
void conn_abort(char* msg)
{
    T(commu.c::conn_abort)
    printf("Protokollfehler: %s\n", msg);
    connstate=conn_no;
    close_connection();
}

#ifdef SELECTCOMM
/* abort_conn ist in sockselectcomm.c definiert */
#define conn_abort(s) abort_conn(s)
#endif
/*****************************************************************************/
errcode init_comm(char* port)
{
    T(commu.c::init_comm)
#ifdef OUT_BUFBEG
    outbuf=map_memory(OUT_BUFBEG,OUT_MAX,&om);
#else /*OUT_BUFBEG  */
#ifdef OSK
    outbuf=(int*)(((int)(_outbuf+3))&0xfffffffc); /* Alignment erzwingen */
#else /* OSK */
#ifdef LOWLEVELBUFFER
    outbuf=lowbuf_outbuffer();
#else /* LOWLEVELBUFFER */
    outbuf=_outbuf;
#endif /* LOWLEVELBUFFER */
#endif /* OSK */
#endif /* OUT_BUFBEG */

    header=(msgheader*)outbuf;
    outbuf+=sizeof(msgheader)/4; /* direkt aneinanderhaengen */
    connstate=conn_no;
    return _init_comm(port);
}
/*****************************************************************************/
errcode done_comm(){
    T(commu.c::done_comm);
#ifdef OUT_BUFBEG
    unmap_memory(&om);
#endif
    return _done_comm();
}
/*****************************************************************************/
static void free_data(unsigned int* buf)
{
    T(commu.c::free_data);
    if ((buf)&&(buf!=quickbuf)) free((char*)buf);
}
/*****************************************************************************/
static int read_data(unsigned int size, ems_u32** buf)
{
    T(commu.c::read_data);
    if(size) {
        if (size<=COMM_QUICKBUFSIZE) {
            *buf=quickbuf;
        } else {
            *buf=(ems_u32*)malloc(size*4);
            if (!*buf) {
	        printf("commu: kein Speicher\n");
	        return -1;
            }
        }
        for (;;) {
            int res;
            if ((res=recv_data(*buf,size))<=0) {
                printf("res=%d; errno=%s\n", res, strerror(errno));
	        if (!res) { /* interrupted */
                    continue;
	        } else{
	            conn_abort("recv error");
	            free_data(*buf);
	            return -1;
	        }
            } else {
	        if (res!=size) {
	            conn_abort("bad size");
	            free_data(*buf);
	            return -1;
	        }
	        break; /* OK */
            }
        }
    } else {
        *buf=(unsigned int*)0;
    }
    return 0;
}
/*****************************************************************************/
static int handle_intmsg(void)
{
    ems_u32 *tmpbuf;

    T(commu.c::handle_intmsg)
    switch (header->type.intmsgtype) {
    case Intmsg_Open:
        if (connstate!=conn_opening) {
	    conn_abort("unexpected Intmsg_Open");
	    return -1;
        }
        D(D_COMM,printf("connection established\n");)
	connstate=conn_ok;
        break;
    case Intmsg_Close:
        if (connstate!=conn_ok) {
	    conn_abort("unexpected Intmsg_Close");
	    return -1;
        }
        D(D_COMM,printf("closing connection\n");)
	connstate=conn_closing;
        break;
    default:
        conn_abort("unexpected Intmsg");
        return -1;
    }

    /* hau wech */
    if (read_data(header->size,&tmpbuf)<0)
        return(-1);
    free_data(tmpbuf);

    header->flags|=Flag_Confirmation;
    header->size=0;
    if (send_data((ems_u32*)header,sizeof(msgheader)/4)<0) {
        conn_abort("send error");
        return -1;
    }
    if (connstate==conn_closing) {
        D(D_COMM,printf("connection closed\n");)
        close_connection();
        connstate=conn_no;
#ifdef SELECTCOMM
        restart_accept();
#endif
    }
    return 0;
}
/*****************************************************************************/

Request wait_indication(ems_u32** reqbuf, unsigned int* size)
{
  T(commu.c::wait_indication)
  for(;;){
    int res;
    if(connstate==conn_no){
      if((res=open_connection())<=0){
	if(!res)return(Req_Nothing); /* interrupted */
	/* error handling ??? */
	continue;
      }
      connstate=conn_opening;
    }
    if((res=recv_data((ems_u32*)header,sizeof(msgheader)/4))<=0){
      if(!res)return(Req_Nothing); /* interrupted */
      conn_abort("recv error");
      continue;
    }
    if(res!=sizeof(msgheader)/4){
      conn_abort("bad size");
      continue;
    }

    if(header->flags&Flag_Intmsg){
      if(handle_intmsg()<0)continue;
    }else{ /* kein Intmsg */
      if(connstate!=conn_ok){
	conn_abort("Intmsg expected");
	continue;
      }
      *size=header->size;
      if(read_data(*size,reqbuf)<0)continue;
      return(header->type.reqtype);
    } /* endif Intmsg */
  } /* forever */
}
/*****************************************************************************/

Request poll_indication(ems_u32** reqbuf, unsigned int* size)
{
    int res;

    T(commu.c::poll_indication)
    if(connstate==conn_no){
        if (poll_connection()<=0) /* XXX also on error */
            return(Req_Nothing);
        connstate=conn_opening;
    }
    if ((res=poll_data((unsigned int*)header, sizeof(msgheader)/4))<=0) {
        if (!res)
            return Req_Nothing;
        conn_abort("poll error");
        return Req_Nothing;
    }
    if (res!=sizeof(msgheader)/4) {
        conn_abort("bad size");
        return Req_Nothing;
    }

    if (header->flags&Flag_Intmsg) {
        handle_intmsg();
        return Req_Nothing;
    }

   if (connstate!=conn_ok) {
        conn_abort("Intmsg expected");
        return Req_Nothing;
    }
    *size=header->size;
    if (read_data(*size,reqbuf)<0)
        return Req_Nothing;
    return header->type.reqtype;
}
/*****************************************************************************/
Request select_indication(ems_u32** reqbuf, unsigned int* size)
{
    int res;

    T(commu.c::select_indication)
    if ((res=recv_data((ems_u32*)header, sizeof(msgheader)/4))<=0) {
        if (res<0) conn_abort("recv error");
    }   else if (res!=sizeof(msgheader)/4) {
        conn_abort("bad header size");
    } else if (header->flags&Flag_Intmsg) {
        handle_intmsg();
    } else {
        if (connstate!=conn_ok) {
            conn_abort("Intmsg_Open expected");
        } else {
            *size=header->size;
            if (read_data(*size, reqbuf)>=0)
                return (header->type.reqtype);
        }
    }
    return Req_Nothing;
}
/*****************************************************************************/
void send_response(unsigned int size)
{
    T(commu.c::send_response)
    if (size<0) {
        printf("wrong size (%d) in send_response\n", size);
        return;
    }
    header->flags |=Flag_Confirmation;
    header->size=size;

    /* outbuf heangt dran */
    if (send_data((ems_u32*)header, sizeof(msgheader)/4+size)<0) {
        printf("send_response: call conn_abort(); size=%d\n", size);
        conn_abort("send error");
    }
}
/****************************************************************************/
void free_indication(ems_u32* ptr)
{
    T(commu.c::free_indication)
    free_data(ptr);
}

/*****************************************************************************/
int commu_outsize(void)
{
    T(commu.c::commu_outsize)
    return OUT_MAX-sizeof(msgheader)/4;
}
/*****************************************************************************/
/*****************************************************************************/
