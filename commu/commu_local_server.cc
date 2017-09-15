/*
 * commu_local_server.cc
 * 
 * created 29.07.94
 * 11.09.1998 PW: regulaer changed to policies
 */

#include "commu_local_server.hxx"
#include "versions.hxx"

VERSION("2009-Aug-21", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_local_server.cc,v 2.13 2009/08/21 22:02:28 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_local_server::C_local_server(const char *name, en_policies policy)
:C_server(name, policy), outbuf(), client_id(0)
{
TR(C_local_server::C_local_server)
ver_ved=4;
ver_req=ver_requesttab;
ver_unsol=ver_unsolmsgtab;
}

/*****************************************************************************/

void C_local_server::removeclient(C_client* client)
{
TR(C_local_server::removeclient)
C_server::removeclient(client);

if (refcount==0)
  {
  grund=io_serverclose;
  delete this;
  }
}

/*****************************************************************************/

int C_local_server::addmessage(C_message* message, int nodel)
{
int flags;

flags=message->header.flags;
if (pol_show() && !(flags & Flag_NoLog))
    logmessage(message, 1); // alle interessierten clients mit Kopien versorgen
if (flags & Flag_Intmsg)
  do_message_internal(message);
else
  do_message_ems(message);

return(0); /* immer ok */
}

/*****************************************************************************/

void C_local_server::do_message_internal(C_message* message)
{
ems_u32* body;

TR(C_local_server::do_message_internal)
body=message->body;

switch (message->header.type.intmsgtype)
  {
  case Intmsg_Nothing       : clog << "Intmsg_Nothing" << endl; break;
  case Intmsg_ClientIdent   : clog << "Intmsg_ClientIdent" << endl; break;
  case Intmsg_Open          : clog << "Intmsg_Open" << endl; break;
  case Intmsg_Close         : clog << "Intmsg_Close" << endl; break;
  case Intmsg_OpenVED       : clog << "Intmsg_OpenVED" << endl; break;
  case Intmsg_CloseVED      : clog << "Intmsg_CloseVED" << endl; break;
  case Intmsg_GetHandle     : clog << "Intmsg_GetHandle" << endl; break;
  case Intmsg_GetName       : clog << "Intmsg_GetName" << endl; break;
  case Intmsg_SetUnsolicited: clog << "Intmsg_SetUnsolicited:" << endl; break;
  case Intmsg_SetExitCommu  : clog << "Intmsg_SetExitCommu" << endl; break;
  default:                    clog << "Intmsg_default" << endl; break;
  }
}

/*****************************************************************************//*****************************************************************************/
/*****************************************************************************/
