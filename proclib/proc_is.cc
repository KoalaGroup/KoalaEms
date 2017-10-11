/*
 * proclib/proc_is.cc
 * 
 * created: 05.09.95 PW
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "proc_is.hxx"
#include "proc_isstatus.hxx"
#include "ved_errors.hxx"
#include <clientcomm.h>
#include <stdarg.h>
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_is.cc,v 2.16 2014/07/14 15:11:54 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/
C_instr_system::C_instr_system(C_VED* ved, int idx, openmode mode,
    int id)
:ved(ved), ved_id(ved->ved_id()), idx(idx), confmode_(ved->confmode_),
    proclist("proc_is.cc:25 (proclist)", ved, this, ved->capab_proc_list),
    readoutlist("proc_is.cc:26 (readoutlist)", ved, this, ved->capab_proc_list)
{
// cout<<"C_instr_system::C_instr_system(this="<<(void*)this<<", idx="<<idx<<")"
//    <<endl;
    if (mode==notest)
        throw new C_program_error("C_instr_system: illegal openmode");

    int xid;
    C_confirmation* conf;

    if ((xid=::CreateIS(ved_id, idx, id))==-1)
        throw new C_ved_error(ved, EMS_errno, "Can't send CreateIs");
    conf=ved->GetConf(xid);
    if ((conf->buffer(0)!=OK) && ((mode==create) ||
            (conf->buffer(0)!=Err_ISDef))) {
        ostringstream s;
        s << "Can't create IS " << idx;
        throw new C_ved_error(ved, conf, s);
    }
#ifdef XXX
    {
        ostringstream ss();
        ss<<"IS "<<idx;
        //cerr<<"C_instr_system::C_instr_system(IS "<<idx<<")"<<endl;
        is_name=ss.str();
    }
#else
    {
        char s[100];
        sprintf(s, "IS %d", idx);
        is_name=string(s);
    }
#endif
    ved->add_is(this);
    proclist.execmode=immediate;
    readoutlist.execmode=delayed;
}
/*****************************************************************************/
C_instr_system::C_instr_system(C_VED* ved, int idx, openmode mode)
:ved(ved), ved_id(ved->ved_id()), idx(idx), confmode_(ved->confmode_),
    proclist("proc_is.cc:61 (proclist)", ved, this, ved->capab_proc_list),
    readoutlist("proc_is.cc:62 (readoutlist)", ved, this, ved->capab_proc_list)
{
// cout<<"C_instr_system::C_instr_system(this="<<(void*)this<<", idx="<<idx<<")"
//     <<endl;
    if (mode==create)
        throw new C_program_error("C_instr_system: illegal openmode");
    if (mode==open) {
        C_confirmation* conf;
        conf=ved->GetNamelist(Object_is);
        C_inbuf ib(conf->buffer(), conf->size());
        delete conf;
        ib++;
        int num=ib.getint();
        int found=0;
        for (int i=0; !found && (i<num); i++) {
            if ((int)ib.getint()==idx)
                found=1;
        }
        if (!found) {
            ostringstream s;
            s << "IS " << idx << " does not exist.";
            throw new C_ved_error(ved, s);
        }
    }
    {
        ostringstream ss;
        ss<<"IS "<<idx;
        //cerr<<"C_instr_system::C_instr_system(IS "<<idx<<")"<<endl;
        is_name=ss.str();
    }
    ved->add_is(this);
    proclist.execmode=immediate;
    readoutlist.execmode=delayed;
}
/*****************************************************************************/
C_instr_system::~C_instr_system()
{
    ved->delete_is(this);
}
/*****************************************************************************/

exec_mode C_instr_system::execution_mode(exec_mode mode)
{
exec_mode res=proclist.execmode;
proclist.execmode=mode;
return(res);
}

/*****************************************************************************/

C_confirmation* C_instr_system::command(C_proclist* list)
{
int xid;
xid=list->send_request(idx);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
    {
    ostringstream s;
    s << "Error in proclist";
    throw new C_ved_error(ved, conf, s);
    }
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_confirmation* C_instr_system::command(const char* proc, int argnum, ...)
{
va_list vl;
int i;

if (proclist.execmode==immediate) proclist.clear();

if (proclist.add_proc(proc)==-1)
  {
  ostringstream s;
  s << "unknown procedure \"" << proc << "\"";
  throw new C_ved_error(ved, s);
  }

va_start(vl, argnum);
for (i=0; i<argnum; i++) proclist.add_par(va_arg(vl, int));
va_end(vl);

if (proclist.execmode==immediate)
  {
  int xid;
  xid=proclist.send_request(idx);
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=ved->GetConf(xid);
    if (conf->buffer(0)!=OK)
      {
      ostringstream s;
      s << "Error in \"" << proc << "\"";
      throw new C_ved_error(ved, conf, s);
      }
    return(conf);
    }
  else
    {
    last_xid_=xid;
    return(0);
    }
  }
else
  return(0);
}

/*****************************************************************************/

C_confirmation* C_instr_system::command(const char* proc,
    const C_outbuf& outbuf)
{
int i;

if (proclist.execmode==immediate) proclist.clear();

if (proclist.add_proc(proc)==-1)
  {
  ostringstream s;
  s << "unknown procedure \"" << proc << "\"";
  throw new C_ved_error(ved, s);
  }

for (i=0; i<outbuf.index(); i++) proclist.add_par(outbuf[i]);

if (proclist.execmode==immediate)
  {
  int xid;
  xid=proclist.send_request(idx);
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=ved->GetConf(xid);
    if (conf->buffer(0)!=OK)
      {
      ostringstream s;
      s << "Error in \"" << proc << "\"";
      throw new C_ved_error(ved, conf, s);
      }
    return(conf);
    }
  else
    {
    last_xid_=xid;
    return(0);
    }
  }
else
  return(0);
}

/*****************************************************************************/

C_confirmation* C_instr_system::command(const char* proc)
{
if (proclist.execmode==immediate) proclist.clear();

if (proclist.add_proc(proc)==-1)
  {
  ostringstream s;
  s << "unknown procedure \"" << proc << "\"";
  throw new C_ved_error(ved, s);
  }

if (proclist.execmode==immediate)
  {
  int xid;
  xid=proclist.send_request(idx);
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=ved->GetConf(xid);
    if (conf->buffer(0)!=OK)
      {
      ostringstream s;
      s << "Error in \"" << proc << "\"";
      throw new C_ved_error(ved, conf, s);
      }
    return(conf);
    }
  else
    {
    last_xid_=xid;
    return(0);
    }
  }
else
  return(0);
}

/*****************************************************************************/

C_confirmation* C_instr_system::command(const char* proc, const char* arg)
{
if (proclist.execmode==immediate) proclist.clear();

if (proclist.add_proc(proc)==-1)
  {
  ostringstream s;
  s << "unknown procedure \"" << proc << "\"";
  throw new C_ved_error(ved, s);
  }

proclist.add_par(arg);

if (proclist.execmode==immediate)
  {
  int xid;
  xid=proclist.send_request(idx);
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=ved->GetConf(xid);
    if (conf->buffer(0)!=OK)
      {
      ostringstream s;
      s << "Error in \"" << proc << "\"";
      throw new C_ved_error(ved, conf, s);
      }
    return(conf);
    }
  else
    {
    last_xid_=xid;
    return(0);
    }
  }
else
  return(0);
}

/*****************************************************************************/

C_confirmation* C_instr_system::command(int proc, int argnum, ...)
{
va_list vl;
int i;

if (proclist.execmode==immediate) proclist.clear();

proclist.add_proc(proc);

va_start(vl, argnum);
for (i=0; i<argnum; i++) proclist.add_par(va_arg(vl, int));
va_end(vl);

if (proclist.execmode==immediate)
  {
  int xid;
  xid=proclist.send_request(idx);
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=ved->GetConf(xid);
    if (conf->buffer(0)!=OK)
      {
      ostringstream s;
      s << "Error in \"" << proc << "\"";
      throw new C_exec_Error(C_ved_error(ved, conf, s));
      }
    return(conf);
    }
  else
    {
    last_xid_=xid;
    return(0);
    }
  }
else
  return(0);
}

/*****************************************************************************/

C_confirmation* C_instr_system::command(int proc)
{
if (proclist.execmode==immediate) proclist.clear();

proclist.add_proc(proc);

if (proclist.execmode==immediate)
  {
  int xid;
  xid=proclist.send_request(idx);
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=ved->GetConf(xid);
    if (conf->buffer(0)!=OK)
      {
      ostringstream s;
      s << "Error in \"" << proc << "\"";
      throw new C_ved_error(ved, conf, s);
      }
    return(conf);
    }
  else
    {
    last_xid_=xid;
    return(0);
    }
  }
else
  return(0);
}

/*****************************************************************************/

void C_instr_system::add_param(int num, int par, ...)
{
va_list vl;
int i;

proclist.add_par(par);
va_start(vl, par);
for (i=0; i<num-1; i++) proclist.add_par(va_arg(vl, int));
va_end(vl);
}

/*****************************************************************************/

C_confirmation* C_instr_system::execute()
{
int xid;
xid=proclist.send_request(idx);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in execute");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_isstatus* C_instr_system::ISStatus()
{
int xid;

if ((xid=::GetISStatus(ved_id, idx))==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't send GetISStatus");
if (confmode_==synchron)
  {
  C_confirmation* conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in GetISStatus");
  const ems_u32* b=(ems_u32*)conf->buffer();
  C_isstatus* status=new C_isstatus(b[1], b[2], b[3], b[4], b+5);
  delete conf;
  return status;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

int C_instr_system::is_id()
{
int xid;

if ((xid=::GetISStatus(ved_id, idx))==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't send GetISStatus");
if (confmode_==synchron)
  {
  C_confirmation* conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in GetISStatus");
  int id=conf->buffer(1);
  delete conf;
  return id;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

void C_instr_system::DownloadISModullist(int modules, ems_u32* list)
{
int xid;
C_confirmation* conf;

if ((xid=::DownloadISModulList(ved_id, idx, modules, list))==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't send DownloadISModullist");
if (confmode_==synchron)
  {
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Can't download IS-Modullist");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

C_memberlist* C_instr_system::UploadISModullist()
{
int xid;
C_confirmation* conf;

if ((xid=::UploadISModulList(ved_id, idx))==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't send upload_is_modullist");
if (confmode_==synchron)
  {
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in UploadISModullist");
  C_inbuf inbuf(conf->buffer(), conf->size());
  delete conf;
  inbuf++; // errorcode
  int num=inbuf.getint();
  int i;
  C_memberlist* memberlist=new C_memberlist(num);
  for (i=0; i<num; i++) memberlist->add(inbuf.getint());
  return(memberlist);
  }
else
  last_xid_=xid;
  return 0;
}

/*****************************************************************************/

void C_instr_system::DeleteISModullist()
{
int xid;
C_confirmation* conf;

if ((xid=::DeleteISModulList(ved_id, idx))==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't send DeleteISModullist");
if (confmode_==synchron)
  {
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Can't delete IS-Modullist");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/
void C_instr_system::ro_add_proc(const char* proc)
{
    if (readoutlist.add_proc(proc)==-1) {
        ostringstream s;
        s << "unknown procedure \"" << proc << "\"";
        throw new C_ved_error(ved, s);
    }
}
/*****************************************************************************/
void C_instr_system::ro_add_proc(const char* proc, int num, ...)
{
    va_list vl;
    int i;

    if (readoutlist.add_proc(proc)==-1) {
        ostringstream s;
        s << "unknown procedure \"" << proc << "\"";
        throw new C_ved_error(ved, s);
    }

    va_start(vl, num);
    for (i=0; i<num; i++)
        readoutlist.add_par(va_arg(vl, ems_u32));
    va_end(vl);
}
/*****************************************************************************/

void C_instr_system::enable(int enab)
{
int xid;

if (enab)
  xid=::EnableIS(ved_id, idx);
else
  xid=::DisableIS(ved_id, idx);
if (xid==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't send {En|Dis}ableIS");
if (confmode_==synchron)
  {
  C_confirmation* conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in {En|Dis}ableIS");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_instr_system::DownloadReadoutlist(int priority, int num_trigg, ...)
{
va_list vl;
int i, xid;
ems_u32* trigglist;
ems_u32* proclist;
int listsize;

trigglist=new ems_u32[num_trigg];

va_start(vl, num_trigg);
for (i=0; i<num_trigg; i++) trigglist[i]=va_arg(vl, int);
va_end(vl);

readoutlist.getlist(&proclist, &listsize);

xid=::DownloadReadoutList(ved_id, idx, priority, num_trigg, trigglist,
    listsize, proclist);
delete[] trigglist;
readoutlist.clear();
if (xid==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't download readoutlist");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in DownloadReadoutlist");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_instr_system::DownloadReadoutlist(int priority, ems_u32* trigg,
    int num_trigg)
{
int xid;
ems_u32* proclist;
int listsize;

readoutlist.getlist(&proclist, &listsize);

xid=::DownloadReadoutList(ved_id, idx, priority, num_trigg, trigg, listsize,
    proclist);
readoutlist.clear();
if (xid==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't download readoutlist");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in DownloadReadoutlist");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_instr_system::DeleteReadoutlist(int trigg)
{
int xid;

xid=::DeleteReadoutList(ved_id, idx, trigg);
if (xid==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't delete readoutlist");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in DeleteReadoutlist");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

C_confirmation* C_instr_system::UploadReadoutlist(int trigg)
{
int xid;

xid=::UploadReadoutList(ved_id, idx, trigg);
if (xid==-1)
    throw new C_ved_error(ved, EMS_errno, "Can't upload readoutlist");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=ved->GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(ved, conf, "Error in UploadReadoutlist");
  return conf;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/
/*****************************************************************************/
