/*
 * commu_io_client.cc      
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
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "commu_log.hxx"
#include "errorcodes.h"
#include "xdrstring.h"
#include "errors.hxx"
#include "commu_server.hxx"
#include "commu_io_server.hxx"
#include "commu_io_client.hxx"
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_io_client.cc,v 2.19 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

extern C_clientlist clientlist;
extern C_serverlist serverlist;
extern C_log elog, nlog, dlog;

/*****************************************************************************/

C_io_client::C_io_client(C_socket *sock)
:hostname(""), inetaddr(0), pid(0), ppid(0), user(""), uid(0), gid(0),
    experiment(""), bigendian(0)
{
    TR(C_io_client::C_io_client)
    socket=sock;
    far=socket->far();
    _setstatus(st_read_ident);
    toread=1;
}

/*****************************************************************************/

void C_io_client::work_read_non_existent()
{
    elog << "C_io_client::work_read_non_existent called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_read_connecting()
{
    elog << "C_io_client::work_read_connecting called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_read_opening()
{
    elog << "C_io_client::work_read_opening called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_read_read_ident()
{
    TR(C_io_client::work_read_read_ident)
    reading.read_(*socket);
    switch (reading.status) {
        case C_io::iost_ready:
            proceed_message(reading.message);
            reading.status=C_io::iost_idle;
            break;
        case C_io::iost_error:
            grund=io_readfehler;
            delete this;
            break;
        case C_io::iost_broken:
            grund=io_readbruch;
            delete this;
            break;
        default:
            {} // XXX what to do here? Error?
    }
}

/*****************************************************************************/

void C_io_client::work_read_error_ident()
{
    elog << "C_io_client::work_read_error_ident called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_read_ready()
{
    TR(C_io_client::work_read_ready)
    reading.read_(*socket);
    switch (reading.status) {
        case C_io::iost_ready:
            proceed_message(reading.message);
            reading.status=C_io::iost_idle;
            break;
        case C_io::iost_error:
            grund=io_readfehler;
            elog << "C_io_client::work_read_ready(): iost_error" << flush;
            delete this;
            break;
        case C_io::iost_broken:
            grund=io_readbruch;
            delete this;
            break;
        default:
            {} // XXX what to do here? Error?
    }
}

/*****************************************************************************/

void C_io_client::work_read_disconnecting()
{
    elog << "C_io_client::work_read_disconnecting called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_write_non_existent()
{
    elog << "C_io_client::work_write_non_existent called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_write_connecting()
{
    elog << "C_io_client::work_write_connecting called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_write_opening()
{
    elog << "C_io_client::work_write_opening called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_write_read_ident()
{
    elog << time << "C_io_client::work_write_read_ident called" << flush;
    delete this;
}

/*****************************************************************************/

void C_io_client::work_write_error_ident()
{
    TR(C_io_client::work_write_error_ident)
    writing.write_(*socket, messagelist);
    switch (writing.status) {
        case C_io::iost_idle:
            if (messagelist.size()==0) {
                grund=io_identfehler;
                delete this;
            }
            break;
        case C_io::iost_error:
            grund=io_writefehler;
            delete this;
            break;
        case C_io::iost_broken:
            grund=io_writebruch;
            delete this;
            break;
        default:
            {} // XXX what to do here? Error?
    }
}

/*****************************************************************************/

void C_io_client::work_write_ready()
{
    TR(C_io_client::work_write_ready)
    writing.write_(*socket, messagelist);
    switch (writing.status) {
        case C_io::iost_idle:
            if (messagelist.size()==0) towrite=0;
            break;
        case C_io::iost_error:
            grund=io_writefehler;
            delete this;
            break;
        case C_io::iost_broken:
            grund=io_writebruch;
            delete this;
            break;
        default:
            {} // XXX what to do here? Error?
    }
}

/*****************************************************************************/
void C_io_client::work_write_disconnecting()
{
    TR(C_io_client::work_write_disconnecting)
    writing.write_(*socket, messagelist);
    switch (writing.status) {
    case C_io::iost_idle:
        if (messagelist.size()==0) {
            grund=io_disconnect;
            delete this;
        }
        break;
    case C_io::iost_error:
        grund=io_writefehler;
        delete this;
        break;
    case C_io::iost_broken:
        grund=io_writebruch;
        delete this;
        break;
    default:
        {} // XXX what to do here? Error?
    }
}
/*****************************************************************************/
void C_io_client::proceed_message(C_message* message)
{
    int flags;

    TR(C_io_client::proceed_message)
    // clog << "C_io_client::proceed_message:" << endl;
    // clog << "size: "    << hex << message->header.size << endl;
    // clog << "client: "  << hex << message->header.client << endl;
    // clog << "ved: "     << hex << message->header.ved << endl;
    // clog << "type: "    << hex << message->header.type.reqtype << endl;
    // clog << "flags: "   << hex << message->header.flags << endl;
    // clog << "transid: " << hex << message->header.transid << endl;
    flags=message->header.flags;
    if (pol_debug() && !(flags & Flag_NoLog))
        logmessage(message, 0);

    if (((int)message->header.client!=identifier) && (status()==st_ready)) {
        elog << "Client unter falschem Namen! (ist " << message->header.client
            << ", soll " << identifier << " sein)" << flush;
        proceed_reject(message, EMSE_Proto);
    }
    if (flags & Flag_Confirmation) {
        delete message;
        elog << "C_io_client::proceed_message: Protokollfehler (Confirmation)!"
            << flush;
        proceed_reject(message, EMSE_Proto);
    } else if (flags & Flag_Intmsg) {
      proceed_message_internal(message);
    } else {
        proceed_message_ems(message);
    }
}
/*****************************************************************************/

void C_io_client::proceed_reject(C_message* message, EMSerrcode error)
{
msgheader *header;
ems_u32 *body;

TR(C_io_client::proceed_reject)
header=&message->header;
body=new ems_u32[1];
body[0]=error;
header->flags|=Flag_Confirmation;
header->ved=EMS_commu;
addmessage(new C_message(header, body), 0);
delete message;
}

/*****************************************************************************/

void C_io_client::proceed_message_internal(C_message* message)
{
msgheader *header;

TR(C_io_client::proceed_message_internal)
if (pol_show() && !(message->header.flags & Flag_NoLog)) logmessage(message, 1);

header=&message->header;
switch (status())
  {
  case st_read_ident:
    {
    switch (header->type.intmsgtype)
      {
      case Intmsg_Nothing: delete message; break;
      case Intmsg_ClientIdent: proceed_ident(message); break;
      default:
        {
        elog << time << "C_io_client::proceed_message_internal: "
            << "unknown requesttype " << header->type.intmsgtype
            << ", status " << statstr() << flush;
        proceed_reject(message, EMSE_Proto);
        }
        break;
      }
    }
    break;
  case st_ready:
    {
    switch (header->type.intmsgtype)
      {
      case Intmsg_Nothing:   delete message; break;
      case Intmsg_Close:     proceed_Close(message); break;
      case Intmsg_OpenVED:   proceed_OpenVED(message); break;
      case Intmsg_CloseVED:  proceed_CloseVED(message); break;
      case Intmsg_GetHandle: proceed_GetHandle(message); break;
      case Intmsg_GetName:   proceed_GetName(message); break;
      case Intmsg_SetUnsolicited: proceed_SetUnsolicited(message); break;
      case Intmsg_SetExitCommu: proceed_SetExitCommu(message); break;
      default:
        {
        elog << "C_io_client::proceed_message_internal: "
            << "unknown requesttype " << header->type.intmsgtype
            << ", status " << statstr() << flush;
        proceed_reject(message, EMSE_Proto);
        }
        break;
      }
    }
    break;
  default:
    {
    elog << "C_client::proceed_message_internal:" << flush;
    elog << "invalid status " << statstr() << ", type "
        << header->type.intmsgtype << flush;
    proceed_reject(message, EMSE_Proto);
    }
    break;
  }
}

/*****************************************************************************/
void C_io_client::proceed_message_ems(C_message* message)
{
    msgheader *header;
    int ved, id, res;
    C_server *server;

    TR(C_io_client::proceed_message_ems)
    header=&message->header;
    if (status()!=st_ready) {
        elog << "C_io_client::proceed_message_ems: nicht st_ready" << flush;
        proceed_reject(message, EMSE_Proto);
        return;
    }

    ved=header->ved;
    if (ved==(int)EMS_commu) {
        proceed_reject(message, EMSE_IllVEDHandle);
        return;
    }

    if ((id=servers.global(ved))==-1) {
        ems_u32 *body;
        body=new ems_u32[1];
        body[0]=Err_NoVED;
        header->size=1;
        header->flags|=Flag_Confirmation;
        addmessage(new C_message(header, body), 0);
        delete message;
        return;
    }

    if ((server=serverlist.get(id))==0) {
        elog << "Programmfehler in C_client::proceed_message_ems" << flush;
        elog << "client has reference to nonexisting server" << flush;
        proceed_reject(message, EMSE_Unknown);
        return;
    }

    res=server->addmessage(message, 1);
    if (res==-1) {
        elog << "proceed_message_ems: addmessage=-1" << flush;
        proceed_reject(message, EMSE_CommMem);
    }
}
/*****************************************************************************/
/*
request:

const int 0;
string hostname;
int ipaddr;
int pid, ppid;
string user;
int uid, gid;
string experiment;
boolean hidden;
boolean bigendian;

confirmation:

int client_id;
int ver_intmsgtab;
*/

void C_io_client::proceed_ident(C_message* message)
{
    msgheader *header;
    ems_u32 *body;
    int res, add;

    TR(C_io_client::proceed_ident)
    header=&message->header;
    res=EMSE_OK;

    if (status()!=st_read_ident) {
        elog << "C_io_client::proceed_ident:" << flush;
        elog << "  Protokollfehler: falscher Status: " << statstr() << flush;
        res=EMSE_Proto;
        add=1;
    } else if (header->size<1) {
        elog << "C_io_client::proceed_ident:" << flush;
        elog << "Protokollfehler: request ist leer" << flush;
        res=EMSE_Proto;
        add=2;
    } else if (message->body[0]!=0) {
        elog << "C_io_client::proceed_ident:" << flush;
        elog << "Client nach altem Stil" << flush;
        res=EMSE_Proto;
        add=3;
    } else {
        try {
            C_inbuf inbuf(message->body, header->size);
            inbuf++;
            int pol;
            inbuf >> hostname >> inetaddr >> pid >> ppid >> user >> uid >> gid
                    >> experiment >> pol >> bigendian >> def_unsol;
            policies=(en_policies)pol;
            ostringstream ss;
            ss << (far?hostname.c_str():"local") << '_' << pid << ends;
            setname(ss.str());
            if (far)
                identifier=(pid&0xffff) | ((inetaddr<<16)&0xffff0000);
            else
                identifier=pid;

            clientlist.counter();
            if ((policies&pol_noshow)==0)
                nlog << time << "connect from client " << name() << flush;
        }
        catch (C_error* e) {
            elog << "C_io_client::proceed_ident:" << flush;
            elog << "  parse identification: " << *e << flush;
            res=EMSE_Proto;
            add=4;
        }
    }

    if (res==EMSE_OK) {
        body=new ems_u32[3];
        header->size=3;
        body[0]=res;
        body[1]=identifier;
        body[2]=ver_intmsgtab;
    } else {
        body=new ems_u32[2];
        header->size=2;
        body[0]=res;
        body[1]=add;
    }
    header->flags|=Flag_Confirmation;

    addmessage(new C_message(header, body), 0);
    if (res==EMSE_OK)
        _setstatus(st_ready);
    else {
        _setstatus(st_error_ident);
        toread=0;
    }
    delete message;
}

/*****************************************************************************/

void C_io_client::proceed_Close(C_message* message)
{
msgheader *header;

TR(C_io_client::proceed_Close)
header=&message->header;

header->size=0;
header->flags|=Flag_Confirmation;
addmessage(new C_message(header, (ems_u32*)0), 0);
delete message;
/*
size=servers.size();
for (i=0; i<size; i++)
  {
  server=serverlist.find_id(servers[i]->global_id);
  if (server==0)
    {
    elog << "Programmfehler in C_client::proceed_Close" << flush;
    elog << "client has reference to nonexisting server" << flush;
    }
  else
    {
    server->removeclient(identifier);
//    if (server->to_close()) serverlist.tfree(server);
    }
  }
servers.allfree();
*/
_setstatus(st_disconnecting);
}

/*****************************************************************************/

void C_io_client::get_io_info(const char** host, int* addr, const char** _user,
        int* _uid, int* _gid, const char** exper, int* bigend) const
{
*host=hostname.c_str();
*addr=inetaddr;
*_user=user.c_str();
*_uid=uid;
*_gid=gid;
*exper=experiment.c_str();
*bigend=bigendian;
}

/*****************************************************************************/
/*****************************************************************************/
