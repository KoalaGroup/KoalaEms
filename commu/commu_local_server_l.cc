/*
 * commu_local_server_l.cc
 * 
 * created 05.02.95
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 * 11.09.1998 PW: regulaer changed to policies
 */

#include "config.h"
#include "cxxcompat.hxx"
#include "commu_server.hxx"
#include "commu_client.hxx"
#include "commu_io_client.hxx"
#include "commu_local_server_l.hxx"
#include <objecttypes.h>
#include <commu_cdb.hxx>
#include <ved_addr.hxx>
#include <xdrstring.h>
#include "xdrstrdup.hxx"
#include "commu_log.hxx"
#include <errors.hxx>
#include "versions.hxx"

VERSION("2009-Aug-21", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_local_server_l.cc,v 2.23 2009/08/21 22:02:28 wuestner Exp $")
#define XVERSION

extern C_db* cdb;
extern C_serverlist serverlist;
extern C_clientlist clientlist;
extern C_ints globalunsollist;
extern C_log elog, nlog, dlog;

/*****************************************************************************/

C_local_server_l::C_local_server_l(const char *name, en_policies policy)
:C_local_server(name, policy), addr(name)
{
TR(C_local_server_l::C_local_server_l)
}

/*****************************************************************************/

C_local_server_l::~C_local_server_l()
{
TR(C_local_server_l::~C_local_dserver_c)
}

/*****************************************************************************/

void C_local_server_l::logstring(const STRING&)
{
throw new C_program_error("C_local_server_l::logstring");
}

/*****************************************************************************/

errcode C_local_server_l::Nothing(const ems_u32* ptr, int num)
{
return(OK);
}

/*****************************************************************************/

errcode C_local_server_l::Initiate(const ems_u32* ptr, int num)
{
if (num!=1) return(Err_ArgNum);
// ptr[0] (ved_id) wird ignoriert
outbuf << ver_ved << ver_req << ver_unsol;
return(OK);
}

/*****************************************************************************/

errcode C_local_server_l::Conclude(const ems_u32* ptr, int num)
{
if (num!=0) return(Err_ArgNum);
delete this;
return(OK);
}

/*****************************************************************************/

errcode C_local_server_l::Identify(const ems_u32* ptr, int num)
{
int x, n;
char s[64];

if (num!=1) return(Err_ArgNum);
if ((unsigned)ptr[0]>2) return(Err_ArgRange);
outbuf << ver_ved << ver_req << ver_unsol << space_(x);
n=0;
if (ptr[0]>0)
  {
  outbuf << "commu"; n++;
  outbuf << strcat(strcat(strcat(s, __DATE__), "; "), __TIME__); n++;
  }
if (ptr[0]>1)
  {
  outbuf << "Configuration folgt spaeter";
  n++;
  }
outbuf << put(x, n);
return(OK);
}

/*****************************************************************************/

errcode C_local_server_l::GetNameList(const ems_u32* ptr, int num)
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

errcode C_local_server_l::GetCapabilityList(const ems_u32* ptr, int num)
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

errcode C_local_server_l::GetProcProperties(const ems_u32* ptr, int num)
{
unsigned int i;

for (i=0; i<ptr[1]; i++)
  {
  if ((int)ptr[i+2]>=NrOfProcs)
    {
    outbuf << ptr[i+2];
    return(Err_ArgRange);
    }
  }
outbuf << ptr[0];
outbuf << ptr[1];
for (i=0; i<ptr[1]; i++)
  {
  int proc_id;
  procprop* prop;

  proc_id=ptr[i+2];
  outbuf << proc_id;
  prop=(this->*Prop[proc_id].prop_proc)();
  outbuf << prop->varlength;  /* (0: constant, 1: variable) */
  outbuf << prop->maxlength;
  if (ptr[0]>0)
    {
    if (prop->syntax)
      outbuf <<prop->syntax;
    else
      outbuf << "";
    }
  if (ptr[0]>1)
    {
    if (prop->text)
      outbuf << prop->text;
    else
      outbuf << "";
    }
  }
return(OK);
}

/*****************************************************************************/

errcode C_local_server_l::DoCommand(const ems_u32* ptr, int len)
{
errcode res;

if (len<2) return(Err_ArgNum);
if (ptr[0]!=0) return(Err_NoIS);

if ((res=test_proclist(ptr+1, len))!=OK) return(res);
return(scan_proclist(ptr+1));
}

/*****************************************************************************/

errcode C_local_server_l::TestCommand(const ems_u32* ptr, int len)
{
if (len<2) return(Err_ArgNum);
if (ptr[0]!=0) return(Err_NoIS);

return(test_proclist(ptr+1, len));
}

/*****************************************************************************/

errcode C_local_server_l::test_proclist(const ems_u32* ptr, int len)
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
errcode C_local_server_l::scan_proclist(const ems_u32* ptr)
{
    int i, num;
    plerrcode res;

    i=num= *ptr++;
    while (i--) {
        if ((res=(this->*Proc[*ptr].proc)(ptr+1))) {
            outbuf << num-i << res;
            return(Err_ExecProcList);
        }
        ptr+= *(ptr+1)+2;
    }
    return(OK);
}
/*****************************************************************************/
/*****************************************************************************/

plerrcode C_local_server_l::proc_echo(const ems_u32* ptr)
{
int i, size;

size=ptr[0];
for (i=1; i<=size; i++) outbuf << ptr[i];
return(plOK);
}

plerrcode C_local_server_l::test_proc_echo(const ems_u32* ptr)
{
return(plOK);
}

C_local_server::procprop* C_local_server_l::prop_proc_echo()
{
static procprop prop={1, -1, 0, "schickt alles zurueck"};
return(&prop);
}

const char C_local_server_l::name_proc_echo[]="Echo";
int C_local_server_l::ver_proc_echo=1;

/*****************************************************************************/
// add_all
// ptr[0]: size (==1)
// ptr[1]: C_station::logtype
//
plerrcode C_local_server_l::proc_add_all(const ems_u32* ptr)
{
    int i, num;

    if (client->pol_debug()) client->pol_debug(1);

    // existierende Stationen konfigurieren
    num=serverlist.size();
    for (i=0; i<num; i++) serverlist[i]->setlog(client_id, (logtype)ptr[1]);
    num=clientlist.size();
    for (i=0; i<num; i++) clientlist[i]->setlog(client_id, (logtype)ptr[1]);

    // Templates fuer neue Stationen aendern
    logtempl_in.free(client_id);    // um doppelte Eintraege zu vermeiden
    logtempl_out.free(client_id);
    switch ((logtype)ptr[1]) {
    case log_in:
        cerr<<"add "<<client_id<<" to logtempl_in"<<endl;
        logtempl_in.add(client_id);
        break;
    case log_out:
        cerr<<"add "<<client_id<<" to logtempl_out"<<endl;
        logtempl_out.add(client_id);
        break;
    case log_bi:
        cerr<<"add "<<client_id<<" to logtempl_in/out"<<endl;
        logtempl_in.add(client_id);
        logtempl_out.add(client_id);
        break;
    case log_none:
        {}
        break;
    }
    return(plOK);
}

plerrcode C_local_server_l::test_proc_add_all(const ems_u32* ptr)
{
    if (ptr[0]!=1) return(plErr_ArgNum);
    if ((unsigned int)ptr[1]>3) return(plErr_ArgRange);
    return(plOK);
}

C_local_server::procprop* C_local_server_l::prop_proc_add_all()
{
    static procprop prop={1, 0, "void", "aktiviert Log fuer alle stations"};
    return(&prop);
}

const char C_local_server_l::name_proc_add_all[]="Add_all";
int C_local_server_l::ver_proc_add_all=1;
/*****************************************************************************/
// add_ved
// ptr[0]: size
// ptr[1]: logtype
// ptr[2]: num of VEDs
// ptr[3...]: station-names
//
plerrcode C_local_server_l::proc_add_station(const ems_u32* ptr)
{
C_inbuf ib(ptr);
int num=ib.getint();
logtype type=(logtype)ib.getint();
nlog << "Add_station: num="<<num<<"; type="<<type<<flush;
for (int i=0; i<num; i++)
  {
  STRING name;
  try
    {
    ib >> name;
    }
  catch (C_error* e)
    {
    OSTRINGSTREAM s;
    s << "C_local_server_l::proc_add_station: " << (*e) << ends;
    delete e;
    elog << s << flush;
    return(plErr_ArgNum);
    }
  nlog << "  "<<name;
  // es kann nur client oder server sein
  C_client* client=clientlist.get(name);
  C_server* server=serverlist.get(name);
  if (client)
    {
    nlog << " --> client "<< client->name() << flush;
    client->setlog(client_id, type);
    }
  if (server)
    {
    nlog << " --> server "<< server->name() << flush;
    server->setlog(client_id, type);
    }
  if ((client==0) && (server==0))
    {
    int i;
    nlog << " not found." << flush;
    nlog << "clients:"<<flush;
    for (i=0; i<clientlist.size(); i++) nlog<<"  "<<clientlist[i]->name()<<flush;
    nlog << "server:"<<flush;
    for (i=0; i<serverlist.size(); i++) nlog<<"  "<<serverlist[i]->name()<<flush;
    outbuf << i;
    return plErr_ArgRange;
    }
  }
return(plOK);
}

plerrcode C_local_server_l::test_proc_add_station(const ems_u32* ptr)
{
if (ptr[0]<2) return(plErr_ArgNum);
if ((unsigned int)ptr[1]>3) return(plErr_ArgRange);
// die Strings sind zu kompliziert
return(plOK);
}

C_local_server::procprop* C_local_server_l::prop_proc_add_station()
{
static procprop prop={1, 0, "void", "aktiviert Log fuer eine station"};
return(&prop);
}

const char C_local_server_l::name_proc_add_station[]="Add_station";
int C_local_server_l::ver_proc_add_station=1;

/*****************************************************************************/
// add_commu
// ptr[0]: size
//
plerrcode C_local_server_l::proc_add_commu(const ems_u32* ptr)
{
return(plOK);
}

plerrcode C_local_server_l::test_proc_add_commu(const ems_u32* ptr)
{
if (ptr[0]!=0) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_l::prop_proc_add_commu()
{
static procprop prop={0, 0, "Name", "aktiviert Log fuer commu"};
return(&prop);
}

const char C_local_server_l::name_proc_add_commu[]="Add_commu";
int C_local_server_l::ver_proc_add_commu=1;

/*****************************************************************************/
// globalunsol
// ptr[0]: size==1
// ptr[1]: on/off
//
plerrcode C_local_server_l::proc_globalunsol(const ems_u32* ptr)
{
if (ptr[1])
  {
  if (!globalunsollist.exists(client_id))
     globalunsollist.add(client_id);
  }
else  
  {
  if (globalunsollist.exists(client_id))
     globalunsollist.free(client_id);
  }
return(plOK);
}

plerrcode C_local_server_l::test_proc_globalunsol(const ems_u32* ptr)
{
if (ptr[0]!=1) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_l::prop_proc_globalunsol()
{
static procprop prop={0, 0, "1|0", "activates global unsol. messages"};
return(&prop);
}

const char C_local_server_l::name_proc_globalunsol[]="globalunsol";
int C_local_server_l::ver_proc_globalunsol=1;

/*****************************************************************************/
// globalvedname
// ptr[0]: size==1
// ptr[1]: id
//
plerrcode C_local_server_l::proc_globalvedname(const ems_u32* ptr)
{
C_server* server=serverlist.get(ptr[1]);
if (server)
  {
  outbuf<<server->name();
  return plOK;
  }
else
  return plErr_ArgRange;
}

plerrcode C_local_server_l::test_proc_globalvedname(const ems_u32* ptr)
{
if (ptr[0]!=1) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_l::prop_proc_globalvedname()
{
static procprop prop={0, 0, "id", "returns name of VED with global ID"};
return(&prop);
}

const char C_local_server_l::name_proc_globalvedname[]="globalvedname";
int C_local_server_l::ver_proc_globalvedname=1;

/*****************************************************************************/
/*****************************************************************************/
