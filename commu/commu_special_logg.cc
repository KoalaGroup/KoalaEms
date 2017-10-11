/*
 * commu_special_logg.cc
 * 
 * created before 05.02.95 PW
 */

#include <commu_logg.hxx>
#include <commu_server.hxx>
#include <commu_local_server_l.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_special_logg.cc,v 2.7 2014/07/14 15:12:20 wuestner Exp $")
#define XVERSION

extern C_serverlist serverlist;

using namespace std;

/*****************************************************************************/

C_speciallogger::C_speciallogger(const char* progname)
:C_logger(progname), servers(10, "C_speciallogger_servers")
{}

/*****************************************************************************/

C_speciallogger::~C_speciallogger()
{}

/*****************************************************************************/

void C_speciallogger::put(int prior, const char *s)
{
int i, count, ident;
C_local_server_l* server;

count=servers.size();
for (i=0; i<count; i++)
  {
  ident=servers[i]->a;
  if ((server=(C_local_server_l*)serverlist.get(ident))!=0)
    server->logstring(s);
  else
    servers.free(ident);
  }
return;
}

/*****************************************************************************/

void C_speciallogger::put(int prior, const string& s)
{
    int i, count, ident;
    C_local_server_l* server;

    count=servers.size();
    for (i=0; i<count; i++) {
        ident=servers[i]->a;
        if ((server=(C_local_server_l*)serverlist.get(ident))!=0)
            server->logstring(s);
        else
            servers.free(ident);
    }
    return;
}

/*****************************************************************************/

void C_speciallogger::addserver(int ident)
{
servers.add(ident);
}

/*****************************************************************************/

void C_speciallogger::removeserver(int ident)
{
servers.free(ident);
}

/*****************************************************************************/
/*
void C_speciallogger::message(const msgheader *head, const int *bod,
    int outgoing, const char *sname)
{
int i, count, ident;
msgheader header;
int *body, *ptr;
int error;
const char *empty="";
C_client* client;

if (sname==NULL) sname=empty;
count=statlist.size();
if (count>0)
  {
  error=0;
  header.size=2+strxdrlen(sname)+headersize+head->size;
  header.ved=EMS_commu;
  header.type.unsoltype=Unsol_LogMessage;
  header.flags=Flag_Confirmation|Flag_Unsolicited;
  header.transid=0;
  for (i=0; i<count; i++)
    {
    ident=statlist[i]->a;
    if ((client=clientlist.find(ident))==0)
      error=1;
    else
      {
      header.client=ident;
      body=new int[header.size*4];
      body[0]=1; // Versionsnummer
      body[1]=outgoing;
      ptr=outstring(body+2, sname);
      bcopy((const char*)head, (char*)ptr, sizeof(msgheader));
      bcopy((const char*)bod, (char*)(ptr+headersize), head->size*4);
      client->addmessage(&header, body);
      }
    }
  if (error)
    for (i=0; i<count; i++)
      {
      ident=statlist[i]->a;
      if (clientlist.find(ident)==0) statlist.free_a(ident);
      }
  }
return;
}
*/
/*****************************************************************************/
/*****************************************************************************/
