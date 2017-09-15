/*
 * commu_local_server_i.cc
 * 
 * created 09.11.94
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <stdlib.h>
#include <commu_server.hxx>
#include <commu_client.hxx>
#include <commu_io_client.hxx>
#include <commu_local_server_i.hxx>
#include <objecttypes.h>
#include <commu_cdb.hxx>
#include <ved_addr.hxx>
#include <xdrstring.h>
#include <xdrstrdup.hxx>
#include <commu_log.hxx>
#include <errors.hxx>
#include <linkdate.hxx>
#include <cppdefines.hxx>
#include <compat.h>
#include "versions.hxx"

VERSION("2009-Aug-21", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_local_server_i.cc,v 2.27 2009/08/21 22:02:28 wuestner Exp $")
#define XVERSION

extern C_db* cdb;
extern C_serverlist serverlist;
extern C_clientlist clientlist;
extern C_log elog, nlog, dlog;
extern C_unix_socket mainusock;
extern C_tcp_socket mainisock;

/*****************************************************************************/

C_local_server_i::C_local_server_i(const char *name, en_policies policy)
:C_local_server(name, policy), addr(name)
{
TR(C_local_server_i::C_local_server_i)
}

/*****************************************************************************/

C_local_server_i::~C_local_server_i()
{
TR(C_local_server_i::~C_local_server_i)
}

/*****************************************************************************/

errcode C_local_server_i::Nothing(const ems_u32* ptr, int num)
{
return(OK);
}

/*****************************************************************************/

errcode C_local_server_i::Initiate(const ems_u32* ptr, int num)
{
if (num!=1) return(Err_ArgNum);
// ptr[0] (ved_id) wird ignoriert
outbuf << ver_ved << ver_req << ver_unsol;
return(OK);
}

/*****************************************************************************/

errcode C_local_server_i::Conclude(const ems_u32* ptr, int num)
{
if (num!=0) return(Err_ArgNum);
delete this;
return(OK);
}

/*****************************************************************************/

errcode C_local_server_i::Identify(const ems_u32* ptr, int num)
{
int x, n;

if (num!=1) return(Err_ArgNum);
if ((unsigned)ptr[0]>2) return(Err_ArgRange);
outbuf << ver_ved << ver_req << ver_unsol << space_(x);
n=0;
if (ptr[0]>0)
  {
  outbuf << "commu"; n++;
  outbuf << linkdate; n++;
  }
if (ptr[0]>1)
  {
  for (int i=0; cppdefs[i]!=0; i++)
    {outbuf << cppdefs[i]; n++;}
  OSTRINGSTREAM s1;
  s1 << "commlist: " << cdb->listname() << ends;
  outbuf << s1;
  n++;
  OSTRINGSTREAM s2;
  s2 << "usocket: " << mainusock << ends;
  outbuf << s2;
  n++;
  OSTRINGSTREAM s3;
  s3 << "isocket: " << mainisock << ends;
  outbuf << s3;
  n++;
  }
outbuf << put(x, n);
return(OK);
}

/*****************************************************************************/

errcode C_local_server_i::GetNameList(const ems_u32* ptr, int num)
{
if (num==0) /* Baum aller Objekte gefordert */
  {
  return(Err_ArgNum);
  }
else
  {
  switch ((Object)ptr[0])
    {
    case Object_null:
      {
      if (num>1) return(Err_ArgNum);
      outbuf << 2; // Anzahl
      outbuf << Object_ved;
      outbuf << Object_is;
      return(OK);
      }
    case Object_ved:
      if (num>1) return(Err_ArgNum);
      outbuf << 1;
      outbuf << 0;
      return(OK);
    case Object_is:
      if (num>1) return(Err_ArgNum);
      outbuf << 0;
      return(OK);
    default :  return(Err_IllObject);
    }
  }
}

/*****************************************************************************/

errcode C_local_server_i::GetCapabilityList(const ems_u32* ptr, int num)
{
int i;

if (num!=1) return(Err_ArgNum);
if (ptr[0]!=Capab_listproc) return(Err_IllCapTyp);

outbuf << NrOfProcs;
for (i=0; i<NrOfProcs; i++)
  {
  outbuf << i;
  outbuf << Proc[i].name_proc;
  outbuf << *(Proc[i].ver_proc);
  }
return(OK);
}

/*****************************************************************************/
errcode C_local_server_i::GetProcProperties(const ems_u32* ptr, int num)
{
    unsigned int i;

    for (i=0; i<ptr[1]; i++) {
        if ((int)ptr[i+2]>=NrOfProcs) {
            outbuf << ptr[i+2];
            return(Err_ArgRange);
        }
    }
    outbuf << ptr[0];
    outbuf << ptr[1];
    for (i=0; i<ptr[1]; i++) {
        int proc_id;
        procprop* prop;

        proc_id=ptr[i+2];
        outbuf << proc_id;
        prop=(this->*Prop[proc_id].prop_proc)();
        outbuf << prop->varlength;  /* (0: constant, 1: variable) */
        outbuf << prop->maxlength;
        if (ptr[0]>0) {
            if (prop->syntax)
                outbuf <<prop->syntax;
            else
                outbuf << "";
        }
        if (ptr[0]>1) {
            if (prop->text)
                outbuf << prop->text;
            else
                outbuf << "";
        }
    }
    return(OK);
}
/*****************************************************************************/

errcode C_local_server_i::DoCommand(const ems_u32* ptr, int len)
{
errcode res;

if (len<2) return(Err_ArgNum);
if (ptr[0]!=0) return(Err_NoIS);

if ((res=test_proclist(ptr+1, len))!=OK) return(res);
return(scan_proclist(ptr+1));
}

/*****************************************************************************/

errcode C_local_server_i::TestCommand(const ems_u32* ptr, int len)
{
if (len<2) return(Err_ArgNum);
if (ptr[0]!=0) return(Err_NoIS);

return(test_proclist(ptr+1, len));
}

/*****************************************************************************/

errcode C_local_server_i::test_proclist(const ems_u32* ptr, int len)
{
plerrcode res;
const ems_u32* max;
int anz, i, x1, x2;

res=plOK;
max=ptr+len;
anz= *ptr++;
for (i=0; i<anz; i++)
  {
  outbuf << space_(x1) << space_(x2) << space_for_counter();
  if ((int)(*ptr)>=NrOfProcs)
    {
    outbuf << *ptr;
    res=plErr_NoSuchProc;
    }
  else
    {
    if (ptr+*(ptr+1)+2>max)
      {
      outbuf << *(ptr+1);
      res=plErr_BadArgCounter;
      }
    else
      {
      res=(this->*Proc[*ptr].test_proc)(ptr+1);
      }
    }
  if (res)
    {
    outbuf << put(x1, i+1) << put(x2, res) << set_counter();
    return(Err_BadProcList);
    }
  else
    {
    outbuf.index(x1);
    }
  ptr+= *(ptr+1)+2;
  }
return(OK);
}

/*****************************************************************************/

errcode C_local_server_i::scan_proclist(const ems_u32* ptr)
{
int i, num;
plerrcode res;

i=num= *ptr++;
while (i--)
  {
  if ((res=(this->*Proc[*ptr].proc)(ptr+1)))
    {
    outbuf << num-i << res;
    return(Err_ExecProcList);
    }
  ptr+= *(ptr+1)+2;
  }
return(OK);
}

/*****************************************************************************/
/*****************************************************************************/

plerrcode C_local_server_i::proc_echo(const ems_u32* ptr)
{
int i, size;

size=ptr[0];
for (i=1; i<=size; i++) outbuf << ptr[i];
return(plOK);
}

plerrcode C_local_server_i::test_proc_echo(const ems_u32* ptr)
{
return(plOK);
}

C_local_server::procprop* C_local_server_i::prop_proc_echo()
{
static procprop prop={1, -1, 0, "schickt alles zurueck"};
return(&prop);
}

const char C_local_server_i::name_proc_echo[]="Echo";
int C_local_server_i::ver_proc_echo=1;

/*****************************************************************************/

plerrcode C_local_server_i::proc_strerror(const ems_u32* ptr)
{
outbuf << strerror(ptr[1]);
return plOK;
}

plerrcode C_local_server_i::test_proc_strerror(const ems_u32* ptr)
{
if (ptr[0]!=1) return plErr_ArgRange;
return plOK;
}

C_local_server::procprop* C_local_server_i::prop_proc_strerror()
{
static procprop prop={1, -1, 0, "konvertiert errno zu Strings"};
return(&prop);
}

const char C_local_server_i::name_proc_strerror[]="Strerror";
int C_local_server_i::ver_proc_strerror=1;

/*****************************************************************************/
// VEDnames
// ptr[0]: size (==0)
//
plerrcode C_local_server_i::proc_vednames(const ems_u32* ptr)
{
C_strlist* list;
int i, size;

try
  {
  list=cdb->getVEDlist();
  }
catch (C_error* e)
  {
  elog << "C_local_server_i::proc_vednames: " << (*e) << flush;
  delete e;
  return(plErr_Program);
  }
size=list->size();
outbuf << size;
for (i=0; i<size; i++) outbuf << (*list)[i];
delete list;
return(plOK);
}

plerrcode C_local_server_i::test_proc_vednames(const ems_u32* ptr)
{
if (ptr[0]!=0) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_i::prop_proc_vednames()
{
static procprop prop={1, 0, "void", "listet alle VED-Namen"};
return(&prop);
}

const char C_local_server_i::name_proc_vednames[]="VEDnames";
int C_local_server_i::ver_proc_vednames=1;

/*****************************************************************************/
// VEDnamebyaddress
// ptr[0]: size
// ptr[1...]: VED-address
plerrcode C_local_server_i::proc_vednamebyaddress(const ems_u32* ptr)
{
C_strlist* list;
int i, size, num, x;
C_VED_addr* addr;
C_inbuf inbuf(ptr);
try
  {
  addr=extract_ved_addr(inbuf);
  }
catch (C_error* e)
  {
  elog << (*e) << flush;
  delete e;
  return(plErr_ArgRange);
  }
list=cdb->getVEDlist();
size=list->size();

num=0;
outbuf << space_(x);
for (i=0; i<size; i++)
  {
  C_VED_addr* addr_=cdb->getVED((*list)[i]);
  if (addr_ && (*addr==*addr_))
    {
    num++;
    outbuf << (*list)[i];
    }
  delete addr_;
  }
outbuf << put(x, num);
delete list;
delete addr;
return(plOK);
}

plerrcode C_local_server_i::test_proc_vednamebyaddress(const ems_u32* ptr)
{
// VED-address ist zu kompliziert
return(plOK);
}

C_local_server::procprop* C_local_server_i::prop_proc_vednamebyaddress()
{
static procprop prop={1, 0, "void", "sucht VED zu gegebener Adresse"};
return(&prop);
}

const char C_local_server_i::name_proc_vednamebyaddress[]="VEDnamebyaddress";
int C_local_server_i::ver_proc_vednamebyaddress=1;

/*****************************************************************************/
// VEDaddress
// ptr[0]: size
// ptr[1...]: vedname
//
plerrcode C_local_server_i::proc_vedaddress(const ems_u32* ptr)
{
STRING s;
C_VED_addr* addr;

C_inbuf inbuf(ptr);
try
  {
  inbuf >> s;
  }
catch (C_error* e)
  {
  elog << (*e) << flush;
  delete e;
  return(plErr_ArgRange);
  }
try
  {
  addr=cdb->getVED(s);
  }
catch (C_error* e)
  {
  elog << (*e) << flush;
  delete e;
  outbuf << 0;
  return(plOK);
  }
if (addr)
  {
  outbuf << 1 << *addr;
  delete addr;
  }
else
  outbuf << 0;
return(plOK);
}

plerrcode C_local_server_i::test_proc_vedaddress(const ems_u32* ptr)
{
if (ptr[0]<1) return(plErr_ArgNum);
if (ptr[0]!=xdrstrlen(ptr+1)) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_i::prop_proc_vedaddress()
{
static procprop prop={0, 0, "VED-Name", "Adresse eines VED"};
return(&prop);
}

const char C_local_server_i::name_proc_vedaddress[]="VEDaddress";
int C_local_server_i::ver_proc_vedaddress=1;

/*****************************************************************************/
// VEDinfo
// ptr[0]: size
// [ptr[1...]: vedname]
//
// output:
//   size=0: Liste aller offenen VEDs
//   sonst:
//      ident
//      status
//      ver_ved
//      ver_req
//      ver_unsol
//      typ: adresstyp, adresse
//      if io: writequeue, lostmessages
//
//      referencecount
//      liste aller clients
//      unsol?
//

plerrcode C_local_server_i::proc_vedinfo(const ems_u32* ptr)
{
if (ptr[0]==0)
  {
  int i, size;

  size=serverlist.size();
  outbuf << size;
  for (i=0; i<size; i++) outbuf << serverlist[i]->name();
  return(plOK);
  }
else
  {
  C_server* server;
  STRING s;
  int i, num, size, idx;
  ems_u32 x1, x2, x3;
  const C_ints* unsollist;
  C_inbuf inbuf(ptr);

  try
    {
    inbuf >> s;
    }
  catch (C_error* e)
    {
    OSTRINGSTREAM s;
    s << "C_local_server_i::proc_vedinfo: " << (*e) << ends;
    delete e;
    elog << s << flush;
    return(plErr_ArgNum);
    }
  server=serverlist.get(s);
  if (server==0) return(plErr_ArgRange);
  outbuf << server->ident();
  outbuf << server->status();
  if (server->status()!=st_ready) return(plOK);
  server->getver(&x1, &x2, &x3);
  outbuf << x1 << x2 << x3;
  const C_VED_addr& addr(server->get_addr());
  outbuf << addr;
  size=clientlist.size();
  outbuf << space_(idx);
  for (i=0, num=0; i<size; i++)
    {
    if (clientlist[i]->has_server(server->ident()))
      {
      outbuf << clientlist[i]->name();
      num++;
      }
    }
  outbuf << put(idx, num);
  unsollist=server->getunsollist();
  num=unsollist->size();
  outbuf << num;
  for (i=0; i<num; i++) outbuf << ((*unsollist)[i])->a;
  return(plOK);
  }
}

plerrcode C_local_server_i::test_proc_vedinfo(const ems_u32* ptr)
{
return(plOK);
}

C_local_server::procprop* C_local_server_i::prop_proc_vedinfo()
{
static procprop prop={-1,-1,0,""};
return(&prop);
}

const char C_local_server_i::name_proc_vedinfo[]="VEDinfo";
int C_local_server_i::ver_proc_vedinfo=1;

/*****************************************************************************/
// Clientinfo
// ptr[0]: size
// [ptr[1...]: clientname]
//
// output:
//   size=0: Liste aller vorhandenen clients
//   sonst:
//      ident
//      status
//      remote
//      connection (tcp|unix)
//      hostname
//      inetaddr
//      user (uid, gid)
//      experiment
//      bigendian
//      if io: writequeue, lostmessages
//      liste aller server
//
plerrcode C_local_server_i::proc_clientinfo(const ems_u32* ptr)
{
if (ptr[0]==0)
  {
  int i, size;

  size=clientlist.size();
  outbuf << size;
  for (i=0; i<size; i++) outbuf << clientlist[i]->name();
  return(plOK);
  }
else
  {
  C_client* client;
  char* s;
  int i, num;
  const C_server_infos* servers;

  s=xdrstrdup(ptr+1);
  num=clientlist.size();
#ifdef NO_ANSI_CXX
  for (i=0; (i<num) && (strcmp(clientlist[i]->name(), s)!=0); i++);
#else
  for (i=0; (i<num) && (clientlist[i]->name()!=s); i++);
#endif
  delete[] s;
  if (i==num) return(plErr_ArgRange);
  client=clientlist[i];
  outbuf << client->ident();
  outbuf << client->status();
  if (client->status()!=st_ready) return(plOK);
  outbuf << client->is_remote();
  if (client->is_remote())
    {
    C_io_client* io_client=(C_io_client*)client;
    const char* hostname;
    int inetaddr;
    const char* user;
    int uid, gid;
    const char* experiment;
    int bigendian;

    outbuf << io_client->is_far();
    io_client->get_io_info(&hostname, &inetaddr, &user, &uid, &gid,
        &experiment, &bigendian);
    outbuf << hostname << inetaddr << user << uid << gid << experiment
        << bigendian;
    }
  outbuf<<(int)client->get_policies();
  servers=client->getserverlist();
  num=servers->size();
  outbuf << num;
  for (i=0; i<num; i++)
    {
    C_server* server;
    int ident;

    ident=(*servers)[i]->global_id;
    server=serverlist.get(ident);
    if (server==0)
      outbuf << "????";
    else
      outbuf << server->name();
    }
  return(plOK);
  }
}

plerrcode C_local_server_i::test_proc_clientinfo(const ems_u32* ptr)
{
return(plOK);
}

C_local_server::procprop* C_local_server_i::prop_proc_clientinfo()
{
static procprop prop={-1,-1,0,""};
return(&prop);
}

const char C_local_server_i::name_proc_clientinfo[]="Clientinfo";
int C_local_server_i::ver_proc_clientinfo=1;

/*****************************************************************************/
/*****************************************************************************/
