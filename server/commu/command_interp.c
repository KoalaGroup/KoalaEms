/*
 * commu/command_interp.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: command_interp.c,v 1.13 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <requesttypes.h>
#include "commu.h"
#include "../main/requests.h"
#include "../main/scheduler.h"
#include <rcs_ids.h>

extern ems_u32 *outptr, *outbuf;
#ifndef SELECTCOMM
static taskdescr *t;
#endif

RCS_REGISTER(cvsid, "commu")

/*****************************************************************************/
#ifndef SELECTCOMM
static void wait_command(callbackdata dummy)
{
  int *msg, size;
  Request typ;
  T(command_interp.c::wait_command)

  if((typ=wait_indication(&msg, &size))){
    outptr=outbuf+1;
    if((unsigned int)typ>=NrOfReqs)
      outbuf[0]=Err_IllRequest;
    else
      outbuf[0]=DoRequest[typ](msg, size);
    send_response(outptr-outbuf);
    free_indication(msg);
  }
}
#endif
/*****************************************************************************/
#ifndef SELECTCOMM
static void poll_command(callbackdata dummy)
{
  int *msg, size;
  Request typ;
  T(command_interp.c::poll_command)

  if((typ=poll_indication(&msg, &size))){
    outptr=outbuf+1;
    if((unsigned int)typ>=NrOfReqs)
      outbuf[0]=Err_IllRequest;
    else
      outbuf[0]=DoRequest[typ](msg, size);
    send_response(outptr-outbuf);
    free_indication(msg);
  }
}
#endif
/*****************************************************************************/

void select_command(__attribute__((unused)) int path, enum select_types selected,
        __attribute__((unused)) union callbackdata dummy)
{
ems_u32 *msg;
unsigned int size;
Request typ;
T(command_interp.c::select_command)

if (selected&select_except)
  {
  printf("Error in select_command\n");
  }
else
  {
  if((typ=select_indication(&msg, &size)))
    {
    outptr=outbuf+1;
    if ((unsigned int)typ>=NrOfReqs)
      outbuf[0]=Err_IllRequest;
    else
      outbuf[0]=DoRequest[typ](msg, size);
    send_response((size_t)(outptr-outbuf));
    free_indication(msg);
    }
  }
}

/*****************************************************************************/

int init_commandinterp(void)
{
#ifndef SELECTCOMM
union callbackdata data;
data.p=(void*)0;
#endif
T(command_interp.c::init_commandinterp)

#ifdef SELECTCOMM
if (init_select_comm()<0) return(1);
#else 
t=sched_insert_block_task(wait_command,poll_command,data,1,0,"commandinterpr");
if(!t)return(1);
#endif
return(0);
}

int done_commandinterp(void){
T(commu/command_interp.c::done_commandinterp);
#ifdef SELECTCOMM
done_select_comm();
#else 
sched_remove_task(t);
#endif
return(0);
}
/*****************************************************************************/
/*****************************************************************************/
