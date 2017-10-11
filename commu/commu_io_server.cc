/*
 * commu_io_server.cc
 * 
 * created before 29.07.94
 * 11.09.1998 PW: regulaer changed to policies
 */

#include <errno.h>
#include <stdlib.h>
#include "commu_log.hxx"
#include <xdrstring.h>
#include "commu_cdb.hxx"
#include "commu_io_server.hxx"
#include "commu_client.hxx"
#include <conststrings.h>
#include <ems_errors.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_io_server.cc,v 2.25 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

extern C_clientlist clientlist;
extern C_ints globalunsollist;
extern C_db *cdb;
extern C_log elog, nlog, dlog;

/*****************************************************************************/

C_io_server::C_io_server(const char *name, en_policies policy)
:C_server(name, policy), addr(0)
{
TR(C_io_server::C_io_server)
//cerr<<"C_io_server::C_io_server("<<name<<")"<<endl;
addr=cdb->getVED(C_station::name());
if (addr==0) throw new C_ems_error(EMSE_UnknownVED, name);
open(addr);
}

/*****************************************************************************/

void C_io_server::open(C_VED_addr* addr_)
{
TR(C_io_server::C_io_server)
//cerr<<"C_io_server::open("<<(*addr_)<<")"<<endl;
addr=addr_;

if (addr==0)
  {
  valid=0;
  if (pol_show()) nlog << time << "VED \"" << name() << "\" unknown" << flush;
  throw new C_ems_error(EMSE_UnknownVED, "???");
  }
switch (addr->type())
  {
  case C_VED_addr::local:
    {
    C_VED_addr_unix* addr_;

    addr_=(C_VED_addr_unix*)addr;
    if (pol_show()) nlog<<time<<"connect VED "<<C_station::name()<<" (socket="
        << addr_->sockname() << ")" << flush;
    far=0;
    socket=new C_unix_socket;
    try
      {
      ((C_unix_socket*)socket)->setname(addr_->sockname());
      socket->connect();
      }
    catch(C_error*)
      {
      delete addr;
      valid=0;
      throw;
      }
    }
    break;
  case C_VED_addr::inet:
    {
    C_VED_addr_inet* addr_;

    addr_=(C_VED_addr_inet*)addr;
    if (pol_show()) nlog << time << "connect VED " << C_station::name() << " (host="
        << addr_->hostname() << ", port=" << addr_->port() << ")" << flush;
    far=1;
    socket=new C_tcp_socket;
    try
      {
      ((C_tcp_socket*)socket)->setname(addr_->hostname(), addr_->port());
      //cerr<<"call socket->connect()"<<endl;
      socket->connect();
      //cerr<<"returned from socket->connect()"<<endl;
      }
    catch(C_error*)
      {
      delete addr;
      valid=0;
      throw;
      }
    }
    break;
  case C_VED_addr::inet_path:
    {
    C_VED_addr_inet_path* addr_;

    addr_=(C_VED_addr_inet_path*)addr;
    if (pol_show())
      {
      int num, i;
      num=addr_->numpathes();
      nlog << time << "connect VED " << C_station::name() << " (host="
          << addr_->hostname() << ", port=" << addr_->port() << "pathes:";
      for (i=0; i<num; i++) nlog << " " << addr_->path(i);
      nlog << flush;
      }
    far=1;
    socket=new C_tcp_socket;
    try
      {
      ((C_tcp_socket*)socket)->setname(addr_->hostname(), addr_->port());
      socket->connect();
      }
    catch(C_error*)
      {
      delete addr;
      valid=0;
      throw;
      }
    }
    break;
  case C_VED_addr::notvalid:
  case C_VED_addr::builtin:
  case C_VED_addr::VICbus:
  default:
    if (pol_show()) nlog<<time<<"VED \""<<name()<<"\": unknown addresstype"
        <<flush;
    valid=0;
    delete addr;
    throw new C_ems_error(EMSE_UnknownVED, "unknown addresstype");
  }
towrite=1;
_setstatus(st_connecting);
//cerr<<"C_io_server::open fertig"<<endl;
}

/*****************************************************************************/

C_io_server::~C_io_server()
{
TR(C_io_server::~C_io_server)
delete addr;
}

/*****************************************************************************/

void C_io_server::work_read_non_existent()
{
elog << "C_io_server::work_read_non_existent called" << flush;
delete this;
}

/*****************************************************************************/

void C_io_server::work_read_connecting()
{
elog << "C_io_server::work_read_connecting called" << flush;
delete this;
}

/*****************************************************************************/
void C_io_server::work_read_opening()
{
    TR(C_io_server::work_read_opening)
    reading.read_(*socket);
    switch (reading.status) {
    case C_io::iost_ready:
        proceed_message(reading.message);
        reading.status=C_io::iost_idle;
        break;
    case C_io::iost_error:
        grund=io_readfehler;
        err=reading.error;
        delete this;
        break;
    case C_io::iost_broken:
        elog << "C_io_server::work_read_opening: broken" << flush;
        grund=io_readbruch;
        err=reading.error;
        delete this;
        break;
    default:
        {} // XXX what to do here? Error?
    }
}
/*****************************************************************************/

void C_io_server::work_read_read_ident()
{
elog << "C_io_server::work_read_ident called" << flush;
delete this;
}

/*****************************************************************************/

void C_io_server::work_read_error_ident()
{
elog << "C_io_server::work_read_error_ident" << flush;
delete this;
}

/*****************************************************************************/
void C_io_server::work_read_ready()
{
    TR(C_io_server::work_read_ready)
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
void C_io_server::work_read_disconnecting()
{
    TR(C_io_server::work_read_disconnecting)
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

void C_io_server::work_write_non_existent()
{
elog << "C_io_server::work_write_non_existent" << flush;
delete this;
}

/*****************************************************************************/

void C_io_server::work_write_connecting()
{
int res;
TR(C_io_server::work_write_connecting)

res=socket->getstatus();
if (res!=0)
  {
  int i, count;

  if (pol_show())
      nlog << "connect to VED " << name() << " failed" << error(res) << flush;
  count=clientlist.size();
  for (i=0; i<count; i++) clientlist[i]->server_ok(identifier, res);
  grund=strerror(res);
  delete this;
  }
else
  {
  msgheader header;
  ems_u32 *body;

  socket->connected();
  switch (addr->type())
    {
    case C_VED_addr::local:
    case C_VED_addr::inet:
      header.size=0;
      body=0;
      break;
    case C_VED_addr::inet_path:
      {
      C_VED_addr_inet_path* addr_;
      int num, size, i;
      ems_u32* ptr;

      addr_=(C_VED_addr_inet_path*)addr;
      num=addr_->numpathes();
      for (i=0, size=0; i<num; i++) size+=strxdrlen(addr_->path(i));
      header.size=size;
      body=new ems_u32[size];
      ptr=body;
      for (i=0; i<num; i++) ptr=outstring(ptr, addr_->path(i));
      }
      break;
    default:
      delete this;
      throw new C_program_error(
          "C_io_server::work_write_connecting(): unknown address type");
    }
  header.client=EMS_commu;
  header.ved=EMS_unknown;
  header.type.intmsgtype=Intmsg_Open;
  header.flags=Flag_Intmsg;
  header.transid=0;
  addmessage(new C_message(header, body), 0);
  _setstatus(st_opening);
  toread=1;
  }
}

/*****************************************************************************/
void C_io_server::work_write_opening()
{
    TR(C_io_server::work_write_opening)
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
        elog << "C_io_server::work_write_opening(): broken" << flush;
        grund=io_writebruch;
        delete this;
        break;
        default:
        {} // XXX what to do here? Error?
  }
}
/*****************************************************************************/

void C_io_server::work_write_read_ident()
{
elog << "C_io_server::work_write_read_ident called" << flush;
delete this;
}

/*****************************************************************************/

void C_io_server::work_write_error_ident()
{
elog << "C_io_server::work_write_error_ident called" << flush;
delete this;
}

/*****************************************************************************/
void C_io_server::work_write_ready()
{
    TR(C_io_server::work_write_ready)
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
void C_io_server::work_write_disconnecting()
{
    /* ist identisch mit C_io_server::work_write_ready*/
    TR(C_io_server::work_write_disconnecting)
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

void C_io_server::removeclient(C_client* client)
{
TR(C_io_server::removeclient)

C_server::removeclient(client);

if (refcount==0)
  {
  if (status()!=st_ready)
    {
    grund=io_verzicht;
    delete this;
    }
  else
    {
    msgheader header;
    header.size=0;
    header.client=EMS_commu;
    header.ved=EMS_unknown;
    header.type.intmsgtype=Intmsg_Close;
    header.flags=Flag_Intmsg;
    header.transid=0;
    addmessage(new C_message(header, NULL), 0);
    _setstatus(st_disconnecting);
    }
  }
}

/*****************************************************************************/

void C_io_server::proceed_message(C_message* message)
{
int flags;

TR(C_io_server::proceed_message)
flags=message->header.flags;

if (pol_show() && !(flags & Flag_NoLog)) logmessage(message, 0);

if ((flags & Flag_Confirmation)==0)
  {
  delete message;
  elog << "C_io_server::proceed_message: Protokollfehler(keine Confirmation)!"
      << flush;
  }
else if (flags & Flag_Intmsg)
  proceed_message_internal(message);
else if (flags & Flag_Unsolicited)
  proceed_message_unsol(message);
else
  proceed_message_ems(message);
}

/*****************************************************************************/

void C_io_server::proceed_message_ems(C_message* message)
{
msgheader *header;
int ident;
C_client *client;

TR(C_io_server::proceed_message_ems)
header=&message->header;
if ((status()==st_opening) && (header->type.reqtype==Req_Initiate))
  {
  proceed_Initiate(message);
  return;
  }
if (status()!=st_ready)
  {
  elog  << "C_io_server::proceed_message_ems: nicht st_ready" << flush;
  delete message;
  return;
  }

ident=header->client;
client=clientlist.get(ident);
if (client==NULL)
  {
  elog  << "C_io_server::proceed_message_ems: client " << ident
      << " existiert nicht" << flush;
  delete message;
  return;
  }

client->addmessage(message, 0);
}

/*****************************************************************************/
void C_io_server::proceed_message_unsol(C_message* message)
{
    msgheader *header;
    ems_u32 *body, *nbody;
    int i, count;
    C_client *client;

    TR(C_io_server::proceed_message_unsol)
    header=&message->header;
    body=message->body;
    if (status()!=st_ready) {
        elog << "C_io_server::proceed_message_unsol: nicht st_ready" << flush;
        delete message;
        return;
    }
    if ((header->client!=0) && (header->client!=EMS_commu)) {
        elog << "C_io_server::proceed_message_unsol: client!=EMS_commu" << flush;
        delete message;
        return;
    }
    count=unsollist.size();

    for (i=0; i<count; i++) {
        client=clientlist.get(unsollist[i]->a);
        if (client==0) {
            elog << "C_io_server::proceed_message_unsol: Programmfehler" << flush;
            elog << "server has reference to nonexisting client" << flush;
        } else {
            unsigned int j;
            nbody=new ems_u32[header->size];
            for (j=0; j<header->size; j++) nbody[j]=body[j];
            header->client=unsollist[i]->a;
            client->addunsolmessage(header, nbody, this);
        }
    }

    count=globalunsollist.size();

    for (i=0; i<count; i++) {
        client=clientlist.get(globalunsollist[i]->a);
        if (client==0) {
            elog << "C_io_server::proceed_message_unsol: Programmfehler" << flush;
            elog << "nonexisting client in global unsollist" << flush;
        } else {
            unsigned int j;
            nbody=new ems_u32[header->size];
            for (j=0; j<header->size; j++) nbody[j]=body[j];
            header->client=globalunsollist[i]->a;
            header->flags|=Flag_IdGlobal;
            client->addunsolmessage(header, nbody, this);
        }
    }
    delete message;
}
/*****************************************************************************/

void C_io_server::proceed_message_internal(C_message* message)
{
msgheader *header;

TR(C_io_server::proceed_message_internal)
header=&message->header;
switch (status())
  {
  case st_ready:
    {
    switch (header->type.intmsgtype)
      {
      case Intmsg_Nothing: delete message; break;             /* 0 */
      default:
        {
        elog << "C_server::proceed_message_internal:" << flush;
        elog << "invalid requesttype " << header->type.intmsgtype
            << ", status " << statstr() << flush;
        delete message;
        }
        break;
      }
    }
    break;
  case st_opening:
    {
    switch (header->type.intmsgtype)
      {
      case Intmsg_Nothing: delete message; break;            /* 0 */
      case Intmsg_Open:    proceed_Open(message); break;     /* 2 */
      case Intmsg_Close:                                     /* 3 */
      case Intmsg_OpenVED:   // kann nie auftreten           /* 4 */
      case Intmsg_CloseVED:  // kann nie auftreten           /* 5 */
      case Intmsg_GetHandle: // kann nie auftreten           /* 6 */
      case Intmsg_SetUnsolicited: // kann nie auftreten      /* 7 */
      default:
        {
        elog << "C_server::proceed_message_internal:" << flush;
        elog << "invalid requesttype " << header->type.intmsgtype
            << ", status " << statstr() << flush;
        delete message;
        }
        break;
      }
    }
    break;
  case st_disconnecting:
    {
    switch (header->type.intmsgtype)
      {
      case Intmsg_Nothing: delete message; break;             /* 0 */
      case Intmsg_Close:  proceed_Close(message); break;     /* 3 */
      default:
        {
        elog << "C_server::proceed_message_internal:" << flush;
        elog << "invalid requesttype " << header->type.intmsgtype
            << ", status " << statstr() << flush;
        delete message;
        }
        break;
      }
    }
    break;
  default:
    {
    elog << "C_server::proceed_message_internal:" << flush;
    elog << "invalid status " << statstr() << ", type "
        << header->type.intmsgtype << flush;
    delete message;
    }
    break;
  }
}

/*****************************************************************************/

void C_io_server::proceed_Open(C_message* message)
{
int res, error, i, count;
ostringstream ss;

TR(C_io_server::proceed_Open)
res=0; error=0;
if (message->header.size>0) res=message->body[0];
if (message->header.size>1) error=message->body[1];
delete message;

if (res!=0)
  {
  count=clientlist.size();
  nlog << "C_io_server::proceed_Open: failed" << flush;
  for (i=0; i<count; i++) clientlist[i]->server_ok(identifier, res);

  if (error==0)
    grund=io_openfehler;
  else
    {
    ss << io_openfehler << "; remote error " << error;
    extgrund=grund=ss.str();
    }
  delete this;
  }
else
  {
  ems_u32* body;
  msgheader header;

  body=new ems_u32[1];
  header.size=1;
  header.client=EMS_commu;
  header.ved=identifier;
  header.type.reqtype=Req_Initiate;
  header.flags=0;
  header.transid=0;
  body[0]=identifier;
//  log_message(&header, body, 1, name());
  addmessage(new C_message(header, body), 0);
  }
}

/*****************************************************************************/

void C_io_server::proceed_Initiate(C_message* message)
{
int res, i, count;
ostringstream ss;
msgheader* header;
ems_u32* body;

TR(C_io_server::proceed_Initiate)
res=0;
header=&message->header;
body=message->body;

if (header->size<1)
  {
  elog << "C_io_server::proceed_Initiate: Error in Server \"" << name()
      << "\": message empty" << flush;
  ss << "Protokollfehler im Server" << ends;
  res=EMSE_System;
  }
else
  {
  res=body[0];
  if (res!=0) ss << EMS_errstr((EMSerrcode)res) << " ("<<res<<")" << ends;
  }

if (res==0)
  {
  if (header->size<4)
    {
    elog << "C_io_server::proceed_Initiate: Error in Server \"" << name()
          << "\"" << flush;
    ss << "Protokollfehler im Server" << ends;
    res=EMSE_System;
    }
  else
    {
    ver_ved=body[1];
    ver_req=body[2];
    ver_unsol=body[3];
    }
  }

delete message;

count=clientlist.size();

for (i=0; i<count; i++) clientlist[i]->server_ok(identifier, res);

if (res!=0)
  {
//if (error==0)
//  grund=io_openfehler;
//else
    {
    extgrund=grund=ss.str();
    }
  delete this;
  }
else
  _setstatus(st_ready);
}

/*****************************************************************************/

void C_io_server::proceed_Close(C_message* message)
{
TR(C_server::proceed_Close)
if (message->header.size!=0)
  {
  elog << "C_server::proceed_Close: Programmfehler im Server "
      << name() << flush;
  }
delete message;
grund=io_serverclose;
if (refcount==0)
  delete this;
else
  {
  nlog << "reconnect server " << name() << flush;
  delete socket;
  try
    {
    switch (addr->type())
      {
      case C_VED_addr::local:
        {
        C_VED_addr_unix* addr_;

        addr_=(C_VED_addr_unix*)addr;
        socket=new C_unix_socket;
        ((C_unix_socket*)socket)->setname(addr_->sockname());
        }
        break;
      case C_VED_addr::inet:
      case C_VED_addr::inet_path:
        {
        C_VED_addr_inet* addr_;

        addr_=(C_VED_addr_inet*)addr;
        socket=new C_tcp_socket;
        ((C_tcp_socket*)socket)->setname(addr_->hostname(), addr_->port());
        }
        break;
      default:
        throw new C_program_error(
            "C_io_server::proceed_Close: unknown addresstype");
      }
    socket->connect();
    }
  catch(C_unix_error* e)
    {
    int count, i;
    valid=0;
    count=clientlist.size();
    for (i=0; i<count; i++) clientlist[i]->server_ok(identifier,e->error()/*,0*/);
    elog << (*e) << flush;
    grund=strerror(e->error());
    delete e;
    delete this;
    return;  // ???????
    }
  catch(C_ems_error* e)
    {
    int count, i;
    valid=0;
    count=clientlist.size();
    for (i=0; i<count; i++) clientlist[i]->server_ok(identifier, e->code()/*, 0*/);
    elog << (*e) << flush;
    grund=EMS_errstr(e->code());
    delete e;
    delete this;
    return;  // ???????
    }
  catch(C_error* e)
    {
    int count, i;
    valid=0;
    count=clientlist.size();
//  for (i=0; i<count; i++) clientlist[i]->server_ok(identifier, e->code(), 0);
    for (i=0; i<count; i++) clientlist[i]->server_ok(identifier, -1);
    elog << "unexpected error in C_io_server::proceed_Close: " << (*e) << flush;
    grund="unknown";
    delete e;
    delete this;
    return;  // ???????
    }
  towrite=1;
  _setstatus(st_connecting);
  }
}

/*****************************************************************************/
/*****************************************************************************/
