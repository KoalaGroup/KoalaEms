/*
 * commu_client.cc
 * 
 * created 29.07.94
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <stdlib.h>
#include "commu_def.hxx"
#include "commu_log.hxx"
#include <xdrstring.h>
#include "commu_server.hxx"
#include "commu_io_server.hxx"
#include "commu_local_server.hxx"
#include "commu_local_server_i.hxx"
#include "commu_local_server_a.hxx"
#include "commu_local_server_l.hxx"
#include "commu_client.hxx"
#include "commu_io_client.hxx"
#include <ems_errors.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_client.cc,v 2.29 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

extern C_clientlist clientlist;
extern C_serverlist serverlist;
extern C_ints globalunsollist;
extern C_log elog, nlog, dlog;

extern int autoexit;
extern int allowautoexit;

char C_client::staticstring[1024];

/*****************************************************************************/

C_client::C_client()
:servers(10, "C_client_servers"), def_unsol(0)
{
TR(C_client::C_client)
identifier=EMS_unknown;
clientlist.insert(this);
policies=pol_nodebug;
}

/*****************************************************************************/
C_client::~C_client()
{
    int i, size;

    TR(C_client::~C_client)
    if (grund=="") grund=io_ende;

    if ((identifier!=(int)EMS_unknown) && pol_show()) {
        nlog << time << "disconnect client " << name() << " (" << grund << ')'
            << flush;
    }

    clientlist.remove(this);

    size=clientlist.size();
    for (i=0; i<size; i++) {
        clientlist[i]->setlog(identifier, log_none);
    }

    size=serverlist.size();
    for (i=0; i<size; i++) {
        serverlist[i]->setlog(identifier, log_none);
    }
    logtempl_in.free(identifier);
    logtempl_out.free(identifier);

    size=servers.size();
    for (i=0; i<size; i++) {
        C_server* server;
        if ((server=serverlist.get(servers[i]->global_id))==0) {
            elog << "C_client::~C_client: lost server" << flush;
        } else {
            server->removeclient(this);
        }
    }
    if (globalunsollist.exists(identifier))
        globalunsollist.free(identifier);
    servers.delete_all();
}
/*****************************************************************************/

int C_client::operator < (const C_client& client) const
{
TR(C_client::operator <)
return(client.identifier<client.identifier);
}

/*****************************************************************************/

int C_client::operator > (const C_client& client) const
{
TR(C_client::operator >)
return(client.identifier>client.identifier);
}

/*****************************************************************************/
/* siehe auch OpenVED()
union reply switch (EMSerrcode body_0)                    // 0
  {
  case EMSE_OK:
    union switch (int selector)                           // 1
      {
      case 0: // ok;
        int handle;                                       // 2
        int ved_attributes<>;                             // 3
            // ver_VED;                                   // 4
            // ver_requesttab;                            // 5
            // ver_unsolmsgtab;                           // 6
      case 1: // Fertigmeldung; unsolicited;
        int handle;                                       // 2
        int ved_attributes<>;                             // 3
            // ver_VED;                                   // 4
            // ver_requesttab;                            // 5
            // ver_unsolmsgtab;                           // 6
      }
  case EMSE_InProgress:
    union switch (int selector)                           // 1
      {
      case 0: // in Progress, Fertigmeldung kommt spaeter;
        int handle;                                       // 2
      case 1: // kann nicht auftreten;
        void;
      }
  default:
    union switch (int selector)                           // 1
      {
      case 0: // Versuch zwecklos (meist Name falsch);
        void;
      case 1: // Fertigmeldung; unsolicited;
        int handle ;                                      // 2
      }
  }
*/

void C_client::server_ok(int ident, int res)
{
msgheader header;
ems_u32 *body;
int handle;

TR(C_client::server_ok)
if ((handle=servers.local(ident))==-1) return;
if (servers.valid(handle))
  {
  elog << "C_client::server_ok: server schon valid (id=" << ident
      << ")" << flush;
  return;
  }
if (res==0)
  {
  C_server* server;
  if ((server=serverlist.get(ident))==0)
    {
    elog << "C_client::server_ok: server " << ident << " nicht in serverlist"
        << flush;
    return;
    }

  servers.setvalid(handle, 1);
  if (def_unsol) server->setunsol(identifier, 1);
  body=new ems_u32[7];
  header.size=7;
  body[0]=res;
  body[1]=1;
  body[2]=handle;
  body[3]=3;
  server->getver(body+4, body+5, body+6);
  }
else
  {
  servers.free(handle);
  body=new ems_u32[3];
  header.size=3;
  body[0]=res;
  body[1]=1;
  body[2]=handle;
  }
header.client=identifier;
header.ved=EMS_commu;
header.type.intmsgtype=Intmsg_OpenVED;
header.flags=Flag_Confirmation | Flag_Intmsg | Flag_Unsolicited;
header.transid=0;
addmessage(new C_message(header, body), 0);
}

/*****************************************************************************/

void C_client::server_died(int ident)
{
msgheader header;
ems_u32 *body;
int handle;
C_server* server;

TR(C_client::server_died)

if ((handle=servers.local(ident))==-1) return;

server=serverlist.get(ident);
if (server==0)
    elog << "C_client::server_died: server nicht in serverlist" << flush;

do
  {
  if (servers.valid(handle))
    {
    elog << "C_client: server died." << flush;
    header.size=1;
    header.ved=EMS_commu;
    header.client=identifier;
    header.type.unsoltype=Unsol_ServerDisconnect;
    header.flags=Flag_Confirmation | Flag_Unsolicited;
    header.transid=0;
    body=new ems_u32[1];
    body[0]=handle;
    }
  else
    {
    elog << "C_client: server died and was not (yet) valid." << flush;
    body=new ems_u32[3];
    header.size=3;
    body[0]=server?server->errnum():EMSE_Unknown;
    body[1]=1;
    body[2]=handle;
    header.client=identifier;
    header.ved=EMS_commu;
    header.type.intmsgtype=Intmsg_OpenVED;
    header.flags=Flag_Confirmation | Flag_Intmsg | Flag_Unsolicited;
    header.transid=0;
    }
  servers.free(handle);
  addmessage(new C_message(header, body), 0);
  }
while ((handle=servers.local(ident))!=-1);
}

/*****************************************************************************/

void C_client::bye()
{
msgheader header;

TR(C_client::bye)
header.size=0;
header.ved=EMS_commu;
header.client=identifier;
header.type.unsoltype=Unsol_Bye;
header.flags=Flag_Confirmation | Flag_Unsolicited;
header.transid=0;
elog << "send 'bye' to client "<<name()<< flush;
addmessage(new C_message(header, (ems_u32*)0), 0);
}

/*****************************************************************************/

const char* C_client::getservername(int handle) const
{
TR(C_client::getservername)
switch (handle)
  {
  case EMS_unknown:
    return("unknown");
  case EMS_commu:
    return("commu");
  case EMS_invalid:
    return("invalid");
  default:
    {
    C_server *server;
    int id;

    if ((id=servers.global(handle))==-1)
      {
      return("not known");
      }
    else
      {
      if ((server=serverlist.get(id))==0)
        {
        return("not valid");
        }
      else
        {
        return server->name().c_str();
        }
      }
    }
  }
}

/*****************************************************************************/

void C_client::addunsolmessage(msgheader* header, ems_u32* body,
    const C_server* server)
{
TR(C_client::addunsolmessage)
addunsolmessage(new C_message(header, body), server);
}

/*****************************************************************************/

void C_client::addunsolmessage(C_message* message, const C_server *server)
{
int handle;

TR(C_client::addunsolmessage)

if (message->header.flags&Flag_IdGlobal)
  message->header.ved=server->ident();
else
  {
  if ((handle=servers.local(server->ident()))==-1)
    message->header.ved=EMS_invalid;
  else
    message->header.ved=handle;
  }
addmessage(message, 0);
}

/*****************************************************************************/

void C_client::addunsolmessage(C_message* message, int server)
{
TR(C_client::addunsolmessage)

message->header.ved=server;
addmessage(message, 0);
}

/*****************************************************************************/

int C_client::has_server(int ident) const
{
return (servers.local(ident)!=-1);
}

/*****************************************************************************/
/*
request:
body[0]... : VED-name

reply:
union reply switch (EMSerrcode body_0)         // 0
  {
  case EMSE_OK:
    union switch (int selector)                // 1
      {
      case 0: // ok;
        int handle;                            // 2
        int ved_attributes<>;                  // 3
            // ver_VED;                        // 4
            // ver_requesttab;                 // 5
            // ver_unsolmsgtab;                // 6
      case 1: // Fertigmeldung; unsolicited;
        int handle;                            // 2
        int ved_attributes<>;                  // 3
            // ver_VED;                        // 4
            // ver_requesttab;                 // 5
            // ver_unsolmsgtab;                // 6
      }
  case EMSE_InProgress:
    union switch (int selector)                // 1
      {
      case 0: // in Progress, Fertigmeldung kommt spaeter;
        int handle;                            // 2
      case 1: // kann nicht auftreten;
        void;
      }
  default:
    union switch (int selector)                // 1
      {
      case 0: // Versuch zwecklos (meist Name falsch);
        void;
      case 1: // Fertigmeldung; unsolicited;
        int handle;                            // 2
      }
  }
*/
void C_client::proceed_OpenVED(C_message* message)
{
    msgheader *header;
    ems_u32 *body;
    char *ved_name;
    EMSerrcode res;
    int global_handle, local_handle=0;
    C_server* server;

    TR(C_client::proceed_OpenVED)
    //cerr<<"C_client::proceed_OpenVED"<<endl;
    header=&message->header;
    body=message->body;
    if ((header->size<1) || (header->size<(xdrstrlen(body)))) {
        elog << "C_client::proceed_OpenVED: body too small" << flush;
        res=EMSE_Proto;
        ved_name=NULL;
    } else {
        int ok=1;
        ved_name=new char[xdrstrclen(&body[0])+1];
        extractstring(ved_name, &body[0]);
        if (pol_show())
            nlog << time << "client " << name() << ": open VED "
                    << ved_name << flush;
        server=serverlist.get(ved_name);
        if (server!=NULL) {
            //cerr<<"server "<<ved_name<<" exists."<<endl;
            server->addclient(this);
        } else {
            //cerr<<"server "<<ved_name<<" does not exist."<<endl;
            try {
                if (strcmp(ved_name, COMMU_I_NAME)==0)
                    server=new C_local_server_i(ved_name,
                            (en_policies)(policies&pol_noshow));
                else if (strcmp(ved_name, COMMU_A_NAME)==0)
                    server=new C_local_server_a(ved_name,
                            (en_policies)(policies&pol_noshow));
                else if (strcmp(ved_name, COMMU_L_NAME)==0)
                    server=new C_local_server_l(ved_name,
                            (en_policies)(policies&pol_noshow));
                else
                    server=new C_io_server(ved_name,
                            (en_policies)(policies&pol_noshow));
            } catch(C_ems_error* e) {
                ok=0;
                elog << time << (*e) << flush;
                res=e->code();
                delete e;
            } catch(C_error* e) {
                ok=0;
                elog << time << (*e) << flush;
                if (e->type()==C_error::e_system)
                    res=(EMSerrcode)((C_unix_error*)e)->error();
                else
                    res=EMSE_Unknown;
                delete e;
            }
        }
        if (ok) {
            global_handle=server->ident();
            local_handle=servers.add(global_handle);
            if (server->status()==st_ready) {
                servers.setvalid(local_handle, 1);
                if (def_unsol) server->setunsol(identifier, 1);
                res=EMSE_OK;
                //cerr<<"C_client::proceed_OpenVED: res=EMSE_OK"<<endl;
            } else {
                res=EMSE_InProgress;
                //cerr<<"C_client::proceed_OpenVED: res=EMSE_InProgress"<<endl;
            }
        } else {
            cerr<<"C_client::proceed_OpenVED: not ok, res="<<res<<endl;
        }
    }
    delete[] ved_name;

    if (res==EMSE_InProgress) {
        body=new ems_u32[3];
        body[1]=0;
        body[2]=local_handle;
        header->size=3;
    } else if (res==EMSE_OK) {
        body=new ems_u32[7];
        header->size=7;
        body[1]=0;
        body[2]=local_handle;
        body[3]=3;
        server->getver(body+4, body+5, body+6);
    } else {
        body=new ems_u32[1];
        header->size=1;
    }

    header->flags|=Flag_Confirmation;
    body[0]=res;
    addmessage(new C_message(header, body), 0);
    //cerr<<"confirmation added"<<endl;
    delete message;
}
/*****************************************************************************/
/*
body[0] : VED-handle
*/
void C_client::proceed_CloseVED(C_message* message)
{
msgheader *header;
ems_u32 *body;
int handle, id, res;
C_server *server;

TR(C_client::proceed_CloseVED)
header=&message->header;
body=message->body;
if (header->size!=1)
  {
  elog << "C_client::proceed_CloseVED: size of body != 1" << flush;
  res=EMSE_Proto;
  }
else
  {
  handle=body[0];
  if ((id=servers.global(handle))==-1)
    {
    res=EMSE_IllVEDHandle;
    }
  else
    {
    if ((server=serverlist.get(id))==0)
      {
      elog << "C_client::proceed_CloseVED: Internal Error" << flush;
      elog << "client has reference to nonexisting server (id=" << handle
          << ")" << flush;
      res=EMSE_Unknown;
      }
    else
      {
      if (pol_show())
          nlog << time << "client " << name() << ": close VED " << handle
              << " (" << server->name() << ")" << flush;
      servers.free(handle);
      server->removeclient(this);
      res=EMSE_OK;
      }
    }
  }
header->flags|=Flag_Confirmation;
body=new ems_u32[1];
header->size=1;
body[0]=res;
addmessage(new C_message(header, body), 0);
delete message;
}

/*****************************************************************************/
/*
body[0] : VED-Name
*/
void C_client::proceed_GetHandle(C_message* message)
{
msgheader *header;
ems_u32 *body;
char *ved_name;
int handle, res;
C_server *server;

TR(C_client::proceed_GetHandle)
header=&message->header;
body=message->body;

if ((header->size<1) || (header->size<xdrstrlen(body)))
  {
  elog << "C_client::proceed_GetHandle: body too small" << flush;
  res=EMSE_Proto;
  }
else
  {
  ved_name=new char[xdrstrclen(body)+1];
  extractstring(ved_name, body);
  server=serverlist.get(ved_name);
  delete[] ved_name;
  if (server==NULL)
    res=EMSE_UnknownVED;
  else if ((handle=servers.local(server->ident()))==-1)
    res=EMSE_VEDNotOpen;
  else
    res=EMSE_OK;
  }
if (res==EMSE_OK)
  {
  body=new ems_u32[2];
  header->size=2;
  body[1]=handle;
  }
else
  {
  body=new ems_u32[1];
  header->size=1;
  }
body[0]=res;
header->flags|=Flag_Confirmation;
addmessage(new C_message(header, body), 0);
delete message;
}

/*****************************************************************************/
/*
body[0] : VED-handle
*/
void C_client::proceed_GetName(C_message* message)
{
msgheader *header;
ems_u32 *body;
int res;
C_server* server;
const char *s;

TR(C_client::proceed_GetName)
header=&message->header;
body=message->body;

if (header->size!=1)
  {
  elog << "C_client::proceed_GetName: error in length" << flush;
  res=EMSE_Proto;
  }
else
  {
  int id;
  if ((id=servers.global(body[0]))==-1)
    res=EMSE_VEDNotOpen;
  else
    {
    if ((server=serverlist.get(id))==0)
      {
      elog << "C_client::proceed_GetName: Programmfehler" << flush;
      elog << "client has reference to nonexisting server" << flush;
      res=EMSE_Unknown;
      }
    else
      {
      s=server->name().c_str();
      res=EMSE_OK;
      }
    }
  }
if (res==EMSE_OK)
  {
  int size;
  size=strxdrlen(s)+1;
  body=new ems_u32[size];
  outstring(body+1, s);
  header->size=size;
  }
else
  {
  body=new ems_u32[1];
  header->size=1;
  }
body[0]=res;
header->flags|=Flag_Confirmation;
addmessage(new C_message(header, body), 0);
delete message;
}

/*****************************************************************************/
/*
body[0] : VED-handle
body[1] : value (-1: only return old value; 0: disable; 1: enable)
*/
void C_client::proceed_SetUnsolicited(C_message* message)
{
msgheader *header;
ems_u32 *body;
int val, res;
C_server* server;

TR(C_client::proceed_SetUnsolicited)
header=&message->header;
body=message->body;
if (header->size!=2)
  {
  elog << "C_client::proceed_SetUnsolicited: error in length" << flush;
  res=EMSE_Proto;
  }
else
  {
  int id;
  if ((id=servers.global(body[0]))==-1)
    res=EMSE_VEDNotOpen;
  else
    {
    server=serverlist.get(id);
    if (server==NULL)
      {
      elog << "C_client::proceed_SetUnsolicited: Programmfehler" << flush;
      elog << "client has reference to nonexisting server; id=" << id << flush;
      res=EMSE_Unknown;
      }
    else
      {
      val=server->setunsol(identifier, body[1]);
      if (val!=-1)
        res=EMSE_OK;
      else
        res=EMSE_Unknown;
      }
    }
  }
if (res==EMSE_OK)
  {
  body=new ems_u32[2];
  body[1]=val;
  header->size=2;
  }
else
  {
  body=new ems_u32[1];
  header->size=1;
  }
body[0]=res;
header->flags|=Flag_Confirmation;
addmessage(new C_message(header, body), 0);
delete message;
}

/*****************************************************************************/
/*
body[0] : value (-1: only return old value; 0: disable; 1: enable)
*/
void C_client::proceed_SetExitCommu(C_message* message)
{
    msgheader *header;
    ems_i32 *ibody;
    int val, res;

    TR(C_client::proceed_SetExitCommu)
    header=&message->header;
    ibody=(ems_i32*)message->body;

    if (header->size!=1) {
        elog << "C_client::proceed_SetExitCommu: error in length" << flush;
        res=EMSE_Proto;
    } else {
        val=autoexit;
        if ((ibody[0]>1) || (ibody[0]<-1)) {
            res=EMSE_Range;
        } else {
            if (ibody[0]!=-1) {
                if (allowautoexit) {
                    autoexit=ibody[0];
                    nlog << time << "set autoexit " << (autoexit?"on":"off")
                        << flush;
                } else {
                    nlog << time << "set autoexit " << (autoexit?"on":"off")
                        << " denied" << flush;
                }
            }
            res=EMSE_OK;
        }
    }

    ems_u32 *body=0;
    if (res==EMSE_OK) {
        body=new ems_u32[2];
        body[1]=val;
        header->size=2;
    } else {
        body=new ems_u32[1];
        header->size=1;
    }
    body[0]=res;
    header->flags|=Flag_Confirmation;
    addmessage(new C_message(header, body), 0);
    delete message;
}
/*****************************************************************************/
//
// logmessage:
//   header:
//     size           ...
//     client         ziel
//     ved            wird von C_client::addunsolmessage eingetragen
//     type.unsoltype Unsol_LogMessage
//     flags          Flag_Confirmation | Flag_Unsolicited | Flag_NoLog
//     transid        0
//
//   body:
//     direction
//     vedname
//     clientname
//     originalheader
//     originalbody
//

void C_client::logmessage(const C_message* message, int in_out /* in 0; out 1*/)
{
int num;

C_ints& list=in_out?loglist_out:loglist_in;
if ((num=list.size())==0) return;

int i;
int error=0, ved_id;
C_station* station;
C_client* client;
msgheader header;
C_outbuf buf;
string sname("sMIST"), cname("cMIST");

cname=name();

if ((ved_id=servers.global(message->header.ved))==-1)
  {
  if (message->header.ved==EMS_commu)
    sname="commu_core";
  else
    {
    elog << "C_client::logmessage: ved=" << message->header.ved
        << ": unknown(a)" << flush;
    sname="unknown";
    }
  }
else
  {
  if ((station=serverlist.get(ved_id))!=0)
    sname=station->name();
  else
    {
    elog << "C_client::logmessage: ved=" << message->header.ved
        << ": unknown(b)" <<flush;
    sname="unknown";
    }
  }

buf << ((message->header.flags&Flag_Confirmation)!=0) << sname << cname
    << put((int*)&message->header, headersize)
    << put(message->body, message->header.size);

header.size=buf.index();
header.type.unsoltype=Unsol_LogMessage;
header.flags=Flag_Confirmation | Flag_Unsolicited | Flag_NoLog;
header.transid=0;

C_message mess(&header, buf.getbuffer());

int errclient=-1;
for (i=0; i<num; i++)
  {
  client=clientlist.get(list[i]->a);
  if (client==0)
    {
    // wir merken uns nur den letzten Fehler, irgendwann kommt jeder mal dran
    errclient=i;
    error++;
    }
  else
    {
    C_message* msg=new C_message(mess);
    msg->header.client=client->ident();
    client->addunsolmessage(msg, EMS_commu);
    }
  }

if (error)
  {
  elog << "C_client::logmessage: client " << errclient << " ist weg." << flush;
  loglist_in.free(errclient);
  loglist_out.free(errclient);
  logtempl_in.free(errclient);
  logtempl_out.free(errclient);
  }
}
/*****************************************************************************/
ostream& C_client::print(ostream& os) const
{
os<<"client "<<name();
return os;
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const C_client& rhs)
{
return rhs.print(os);
}
/*****************************************************************************/
/*****************************************************************************/

C_clientlist::C_clientlist(int size, string name)
:C_list<C_client>(size, name), leer_(1)
{
TR(C_clientlist::C_clientlist)
}

/*****************************************************************************/
C_clientlist::~C_clientlist()
{
TR(C_clientlist::~C_clientlist)
}
/*****************************************************************************/

void C_clientlist::counter()
{
int i, count;

TR(C_clientlist::counter)
count=0;
for (i=0; i<firstfree; i++) if (list[i]->pol_count()) count++;
leer_=(count==0);
}

/*****************************************************************************/

void C_clientlist::free(int ident)
{
#if 0
int i, j;
#else
int i;
#endif

TR(C_clientlist::free)
i=0;
while ((i<firstfree) && (ident!=list[i]->ident())) i++;
if (i==firstfree) return;
delete list[i];
#if 0
for (j=i; i<firstfree-1; i++) list[i]=list[i+1];
#else
for (; i<firstfree-1; i++) list[i]=list[i+1];
#endif
firstfree--;
shrinklist();
counter();
}

/*****************************************************************************/

C_io_client* C_clientlist::findsock(int path) const
{
int i;

TR(C_clientlist::findsock)
i=0;
while ((i<firstfree) &&
       (!list[i]->is_remote() || (path!=((C_io_client*)list[i])->socknum())))
  i++;
if (i==firstfree)
  {
  return(0);
  }
else
  {
  return((C_io_client*)list[i]);
  }
}

/*****************************************************************************/

C_client* C_clientlist::get(int ident) const
{
int i;

TR(C_clientlist::find)
i=0;
while ((i<firstfree) && (ident!=list[i]->ident())) i++;
if (i==firstfree)
  return 0;
else
  return list[i];
}

/*****************************************************************************/

C_client* C_clientlist::get(const string& name) const
{
int i;

TR(C_clientlist::get(const string&) const)
i=0;
while ((i<firstfree) && (name!=list[i]->name())) i++;
if (i==firstfree)
  return 0;
else
  return list[i];
}

/*****************************************************************************/

void C_clientlist::readfield(fd_set *set, int& maxpath) const
{
int i;

TR(C_clientlist::readfield)
for (i=0; i<firstfree; i++)
  if (list[i]->is_remote())
    if (((C_io_client*)list[i])->to_read())
      {
      int p=((C_io_client*)list[i])->socknum();
      FD_SET(p, set);
      if (p>maxpath) maxpath=p;
      }
}

/*****************************************************************************/

void C_clientlist::writefield(fd_set *set, int& maxpath) const
{
int i;

TR(C_clientlist::writefield)
for (i=0; i<firstfree; i++)
  if (list[i]->is_remote())
    if (((C_io_client*)list[i])->to_write())
      {
      int p=((C_io_client*)list[i])->socknum();
      FD_SET(p, set);
      if (p>maxpath) maxpath=p;
      }
}

/*****************************************************************************/

void C_clientlist::exceptfield(fd_set *set, int& maxpath) const
{
int i;

TR(C_clientlist::exceptfield)
for (i=0; i<firstfree; i++)
  if (list[i]->is_remote())
    if (((C_io_client*)list[i])->to_write() ||
        ((C_io_client*)list[i])->to_read())
      {
      int p=((C_io_client*)list[i])->socknum();
      FD_SET(p, set);
      if (p>maxpath) maxpath=p;
      }
}

/*****************************************************************************/
/*****************************************************************************/
