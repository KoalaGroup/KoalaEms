/*
 * commu/socket/sockselectcomm.c
 * created      30.04.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sockselectcomm.c,v 1.5 2011/04/06 20:30:21 wuestner Exp $";

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sconf.h>
#include <debug.h>
#include <msg.h>
#include <rcs_ids.h>
#include "../commu.h"
#include "socketcomm.h"
#include "../../main/scheduler.h"

static struct seltaskdescr* t;
void accept_comm_socket(int, enum select_types, union callbackdata);

RCS_REGISTER(cvsid, "commu/socket")

/*****************************************************************************/

void restart_accept()
{
union callbackdata data;
data.p=0;
T(socketselectcomm.c::restart_accept)
if (t) sched_delete_select_task(t);
t=sched_insert_select_task(accept_comm_socket, data, "commandinterpr_accept",
    getlistenpath(), select_read|select_except
#ifdef SELECT_STATIST
    , 1
#endif
    );
if (!t)
  {
  printf("abort_conn: cannot install 'accept_comm_socket'\n");
  printf("server is now unreachable\n");
  }
}

/*****************************************************************************/

void abort_conn(char* s)
{
T(socketselectcomm.c::abort_conn)
conn_abort(s);
restart_accept();
}

/*****************************************************************************/

void accept_comm_socket(int path, enum select_types selected,
        union callbackdata data)
{
int res;
T(socketselectcomm.c::accept_comm_socket)
if (selected&select_except)
  {
  printf("Error in accept_comm_socket(path=%d)\n", path);
  sched_delete_select_task(t); t=0;
  }
else
  {
  res=poll_connection();
  switch (res)
    {
    case 1: /* ok */
      connstate=conn_opening;
      sched_delete_select_task(t);
      t=sched_insert_select_task(select_command, data, "commandinterpr_read",
        getcommandpath(), select_read|select_except
#ifdef SELECT_STATIST
        , 1
#endif
        );
      if (!t) printf("select_command_accept(): task not inserted\n");
      break;
    case 0: /* interrupted or denied */ /* will be ignored */
      break;
    case -1: /* error */ /* will also be ignored */
      break;
    }
  }
}

/*****************************************************************************/

int init_select_comm()
{
union callbackdata data;
data.p=0;
T(socketselectcomm.c::init_select_comm)
t=sched_insert_select_task(accept_comm_socket, data, "commandinterpr_accept",
    getlistenpath(), select_read|select_except
#ifdef SELECT_STATIST
        , 1
#endif
    );
if (t==0)
  return -1;
else
  return 0;
}

void done_select_comm()
{
T(socketselectcomm.c::done_select_comm)
if (t) sched_delete_select_task(t);
}

/*****************************************************************************/
/*****************************************************************************/
