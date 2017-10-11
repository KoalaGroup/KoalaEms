/*
 * commu_server.cc
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
#include "commu_log.hxx"
#include "commu_server.hxx"
#include "commu_io_server.hxx"
#include "commu_client.hxx"
#include "commu_list_t.hxx"
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_server.cc,v 2.23 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

extern C_clientlist clientlist;
extern C_serverlist serverlist;
extern C_log elog, nlog, dlog;

using namespace std;

/*****************************************************************************/

C_server::C_server(const char* name, en_policies policy)
:unsollist(10, "C_server_unsollist"), refcount(1), ver_ved(0), ver_req(0),
  ver_unsol(0)
{
TR(C_server::C_server)
setname(name);
identifier=serverlist.nextid();
serverlist.insert(this);
policies=policy;
valid=1;
}

/*****************************************************************************/

C_server::~C_server()
{
TR(C_server::~C_server)

if (valid)
  {
  int i, count;

  if (grund=="") grund=io_ende;
  if ((policies&pol_noshow)==0)
    nlog << time << "close VED " << name() << " (" << grund << ')' << flush;
//  if (refcount>0)
    {
    count=clientlist.size();
    for (i=0; i<count; i++)
      {
      clientlist[i]->server_died(identifier);
      }
    }
  }
serverlist.remove(this);
}

/*****************************************************************************/

int C_server::operator < (const C_server& server) const
{
TR(C_server::operator <)
return name()<server.name();
}

/*****************************************************************************/

int C_server::operator > (const C_server& server) const
{
TR(C_server::operator >)
return name()>server.name();
}

/*****************************************************************************/

void C_server::addclient(C_client* client)
{
TR(C_server::addclient)

refcount++;
if (client->pol_show()) policies=(en_policies)(policies&~pol_noshow);

DL(
  dlog << level(D_COMM) << "C_server::addclient, Status=" << statstr() << flush;
  dlog << level(D_COMM) << "refcount=" << refcount << flush;
)
//if (status()==st_ready) station->server_ok(this, EMSE_OK, 0);
}

/*****************************************************************************/

void C_server::removeclient(C_client* client)
{
TR(C_server::removeclient)
unsollist.free(client->ident());
refcount--;
}

/*****************************************************************************/

int C_server::setunsol(int client, int val)
{
int oldval;

TR(C_server::setunsol)
oldval=(unsollist.exists(client));
if ((val==-1) || (val==oldval))
  {
  return(oldval);
  }
if (val==1)
  unsollist.add(client);
else
  unsollist.free(client);
return(oldval);
}

/*****************************************************************************/

void C_server::getver(ems_u32* version_ved, ems_u32* version_req,
        ems_u32* version_unsol) const
{
    TR(C_server::getver)
    *version_ved=ver_ved;
    *version_req=ver_req;
    *version_unsol=ver_unsol;
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

void C_server::logmessage(const C_message* message, int in_out /* in 0; out 1*/)
{
    int num;

    C_ints& list=in_out?loglist_out:loglist_in;
    if ((num=list.size())==0) return;

    int i;
    int error=0;
    C_station* station;
    C_client* client;
    msgheader header;
    C_outbuf buf;
    string sname("sMIST"), cname("cMIST");

    if (message->header.client==EMS_commu) {
        cname="commu_core";
    } else if ((station=clientlist.get(message->header.client))!=0) {
        if (!station->pol_debug()) return;
        cname=station->name();
    } else {
        elog << "C_server::logmessage: client=" << message->header.client<<
            " not known" << flush;
        cname="unknown";
    }

    sname=name();

    buf << ((message->header.flags&Flag_Confirmation)!=0) << sname << cname
        << put((int*)&message->header, headersize)
        << put(message->body, message->header.size);

    header.size=buf.index();
    header.type.unsoltype=Unsol_LogMessage;
    header.flags=Flag_Confirmation | Flag_Unsolicited | Flag_NoLog;
    header.transid=0;

    C_message mess(&header, buf.getbuffer());

    int errclient=0;
    for (i=0; i<num; i++) {
        client=clientlist.get(list[i]->a);
        if (client==0) {
            // wir merken uns nur den letzten Fehler,
            // irgendwann kommt jeder mal dran
            errclient=i;
            error++;
        } else {
            C_message* msg=new C_message(mess);
            msg->header.client=client->ident();
            client->addunsolmessage(msg, EMS_commu);
        }
    }

    if (error) {
        elog << "C_server::logmessage: client " << errclient << " ist weg."
            << flush;
        loglist_in.free(errclient);
        loglist_out.free(errclient);
        logtempl_in.free(errclient);
        logtempl_out.free(errclient);
    }
}
/*****************************************************************************/
ostream& C_server::print(ostream& os) const
{
os<<name();
return os;
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const C_server& rhs)
{
return rhs.print(os);
}
/*****************************************************************************/
/*****************************************************************************/

C_serverlist::C_serverlist(int size, string name)
:C_list<C_server>(size, name), last_id(0)
{
TR(C_serverlist::C_serverlist)
}

/*****************************************************************************/
C_serverlist::~C_serverlist()
{
TR(C_serverlist::~C_serverlist)
}
/*****************************************************************************/

void C_serverlist::free(char *name)
{
#if 0
int i, j;
#else
int i;
#endif

TR(C_serverlist::free(char))
i=0;
#ifdef NO_ANSI_CXX
while ((i<firstfree) && (strcmp(name, list[i]->name())!=0)) i++;
#else
while ((i<firstfree) && (name!=list[i]->name())) i++;
#endif
if (i==firstfree)
  {
  return;
  }
delete list[i];
#if 0
for (j=i; i<firstfree-1; i++) list[i]=list[i+1];
#else
for (; i<firstfree-1; i++) list[i]=list[i+1];
#endif
firstfree--;
shrinklist();
}

/*****************************************************************************/

C_io_server* C_serverlist::findsock(int path) const
{
int i;

TR(C_serverlist::findsock)
i=0;
while ((i<firstfree) &&
       (!list[i]->is_remote() || (path!=((C_io_server*)list[i])->socknum())))
  i++;
if (i==firstfree)
  {
  return(0);
  }
else
  {
  return((C_io_server*)list[i]);
  }
}

/*****************************************************************************/

C_server* C_serverlist::get(char* name) const
{
int i;

TR(C_serverlist::[](char*))
i=0;
#ifdef NO_ANSI_CXX
while ((i<firstfree) && (strcmp(name, list[i]->name())!=0)) i++;
#else
while ((i<firstfree) && (name!=list[i]->name())) i++;
#endif
if (i==firstfree)
  {
  return(0);
  }
else
  {
  return(list[i]);
  }
}

/*****************************************************************************/

C_server* C_serverlist::get(const string& name) const
{
int i;

TR(C_serverlist::[](char*))
i=0;
while ((i<firstfree) && (name!=list[i]->name())) i++;
return (i==firstfree)?0:list[i];
}

/*****************************************************************************/

void C_serverlist::readfield(fd_set *set, int& maxpath) const
{
int i;

TR(C_serverlist::readfield)
for (i=0; i<firstfree; i++)
  if (list[i]->is_remote())
    if (((C_io_server*)list[i])->to_read())
      {
      int p=((C_io_server*)list[i])->socknum();
      FD_SET(p, set);
      if (p>maxpath) maxpath=p;
      }
}

/*****************************************************************************/

void C_serverlist::writefield(fd_set *set, int& maxpath) const
{
int i;

TR(C_serverlist::writefield)
for (i=0; i<firstfree; i++)
  if (list[i]->is_remote())
    if (((C_io_server*)list[i])->to_write())
      {
      int p=((C_io_server*)list[i])->socknum();
      FD_SET(p, set);
      if (p>maxpath) maxpath=p;
      }
}

/*****************************************************************************/

void C_serverlist::exceptfield(fd_set *set, int& maxpath) const
{
int i;

TR(C_serverlist::exceptfield)
for (i=0; i<firstfree; i++)
  if (list[i]->is_remote())
    if (((C_io_server*)list[i])->to_write() ||
        ((C_io_server*)list[i])->to_read())
      {
      int p=((C_io_server*)list[i])->socknum();
      FD_SET(p, set);
      if (p>maxpath) maxpath=p;
      }
}

/*****************************************************************************/

int C_serverlist::nextid()
{
return ++last_id;
}

/*****************************************************************************/
C_server* C_serverlist::get(int id) const
{
int i;

TR(C_serverlist::find_id)
i=0;
while ((i<firstfree) && (list[i]->ident()!=id)) i++;
if (i==firstfree)
  return(0);
else
  return(list[i]);
}
/*****************************************************************************/
ostream& C_server_info::print(ostream& os) const
{
os<<global_id<<endl;
return os;
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const C_server_info& rhs)
{
return rhs.print(os);
}
/*****************************************************************************/
int C_server_infos::add(int global)
{
insert(new C_server_info(global, ++last_local_id));
return(last_local_id);
}
/*****************************************************************************/
void C_server_infos::free(int local)
{
int i;
i=0;
while ((i<firstfree) && (list[i]->local_id!=local)) i++;
if (i<firstfree)
  remove_delete(i);
else
  elog << "C_server_infos::free(int): Eintrag " << local << " existiert nicht"
      << flush;
}
/*****************************************************************************/
int C_server_infos::valid(int local) const
{
int i=0;
while ((i<firstfree) && (list[i]->local_id!=local)) i++;
if (i==firstfree)
  {
  ostringstream ss;
  ss << "C_server_infos::valid: cannot find local id " << local << ends;
  throw new C_program_error(ss);
  }
else
  return(list[i]->valid);
}
/*****************************************************************************/
void C_server_infos::setvalid(int local, int val)
{
int i=0;
while ((i<firstfree) && (list[i]->local_id!=local)) i++;
if (i==firstfree)
  {
  ostringstream ss;
  ss << "C_server_infos::setvalid: cannot find local id " << local << ends;
  throw new C_program_error(ss);
  }
else
  list[i]->valid=val;
}
/*****************************************************************************/
int C_server_infos::global(int local) const
{
int i=0;
while ((i<firstfree) && (list[i]->local_id!=local)) i++;
if (i==firstfree)
  return -1;
else
  return list[i]->global_id;
}
/*****************************************************************************/

int C_server_infos::local(int global) const
{
int i=0;
while ((i<firstfree) && (list[i]->global_id!=global)) i++;
if (i==firstfree)
  return -1;
else
  return list[i]->local_id;
}
/*****************************************************************************/
/*****************************************************************************/
