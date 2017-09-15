/*
 * proc_ved.cc
 *
 * created: 16.08.94 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <xdrstring.h>
#include <clusterformat.h>
#include <clientcomm.h>
#include <errors.hxx>
#include <ems_errors.hxx>
#include "ved_errors.hxx"
#include "proc_ved.hxx"
#include "proc_is.hxx"
#include "proc_dataio.hxx"
#include "proc_communicator.hxx"
#include "proc_veds.hxx"
#include "proc_addrtrans.hxx"
#include "compat.h"
#include <versions.hxx>

VERSION("2009-Feb-24", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_ved.cc,v 2.40 2010/06/20 22:44:07 wuestner Exp $")
#define XVERSION

const C_VED::VED_prior C_VED::NO_prior=0;     // auxiliary System
const C_VED::VED_prior C_VED::DEF_prior=-1;   // erzeugt default-Prioritaet
const C_VED::VED_prior C_VED::CC_prior=1000;
const C_VED::VED_prior C_VED::CCM_prior=2000; // mit Master-Trigger
const C_VED::VED_prior C_VED::SER_prior=3000;
const C_VED::VED_prior C_VED::EB_prior=4000;

/*****************************************************************************/

C_VED::C_VED(const STRING& name, C_VED::VED_prior prior)
:ved_name(name), namelist("namelist"),
  namelist_dom("namelist_dom"), namelist_pi("namelist_pi"),
  capab_proc_list("capab_proc_list"), capab_trig_list("capab_trig_list"),
  lamlist("lamlist"), triglist("triglist"), islist(0), num_is(0), is0_(0)
{
    C_confirmation* conf;
    int xid;

    if (!communication.valid())
        communication.init();

    ved=(ems_u32)OpenVED(name.c_str());
    if (ved==(ems_u32)-1) {
        OSTRINGSTREAM s;
        s << "Can't open VED \"" << name <<"\"";
        throw new C_ems_error(EMS_errno, s);
    }
    confmode_=veds.def_confmode();

// get some namelists
    try {
        int req[1]={0};
        if ((xid=::GetNameList(ved, 1, req))==-1)
            throw new C_ved_error((C_VED*)0, EMS_errno, "Can't request namelist");
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error((C_VED*)0, conf, "Error in GetNamelist null");
        namelist=new C_namelist(conf);
    } catch (C_error* e) {
        cout << "GetNameList(0): "<<*e<<endl;
        throw;
    }
    if (namelist->exists(Object_domain)) {
        int req[2]={Object_domain, 0};
    try {
            if ((xid=::GetNameList(ved, 2, req))==-1)
                throw new C_ved_error((C_VED*)0, EMS_errno,
                        "Can't request namelist(Object_domain)");
            conf=GetConf(xid);
            if (conf->buffer(0)!=OK)
                    throw new C_ved_error((C_VED*)0, conf,
                    "Error in GetNamelist(Object_domain)");
            namelist_dom=new C_namelist(conf);
        } catch (C_error* e) {
            cout << "GetNameList(Object_domain): "<<*e<<endl;
            throw;
        }
    }
    if (namelist->exists(Object_pi)) {
        int req[2]={Object_pi, 0};
        try {
            if ((xid=::GetNameList(ved, 2, req))==-1)
                throw new C_ved_error((C_VED*)0, EMS_errno,
                        "Can't request namelist(Object_pi)");
            conf=GetConf(xid);
            if (conf->buffer(0)!=OK)
            throw new C_ved_error((C_VED*)0, conf,
                    "Error in GetNamelist(Object_pi)");
            namelist_pi=new C_namelist(conf);
        } catch (C_error* e) {
            cout << "GetNameList(Object_pi): "<<*e<<endl;
            throw;
        }
    }
// use the namelists
// if (namelist->exists(Object_is))
    {   // VED besitzt IS, ist also so eine Art controller
        // das muss noch ein wenig sortiert werden
        loadcaplist(Capab_listproc, &capab_proc_list);
        loadcaplist(Capab_trigproc, &capab_trig_list);
        is0_=new C_instr_system(this, 0, C_instr_system::notest);
        if (namelist_dom.valid()) {
            if (namelist_dom->exists(Dom_Trigger)) {
                make_triglist();
            }
            if (namelist_dom->exists(Dom_LAMproclist)) {
                make_lamlist();
            }
        }
    }
    veds.add(this, prior);
}
/*****************************************************************************/
C_VED::~C_VED()
{
    CloseVED(ved);
    if (islist) {
        while (num_is) {
            delete islist[0];
        }
        delete[] islist;
    }
    veds.remove(this);
}
/*****************************************************************************/

void C_VED::make_lamlist(void)
{
if (!lamlist)
  {
  lamlist=new C_proclist("proc_ved.cc:183 (lamlist)", this, 0, capab_proc_list);
  if (!lamlist) throw new C_ved_error(this, errno, "Can't create lam list");
  }
}

/*****************************************************************************/

void C_VED::make_triglist(void)
{
if (!triglist)
  {
  triglist=new C_proclist("proc_ved.cc:194 (triglist)", this, 0, capab_trig_list);
  if (!triglist) throw new C_ved_error(this, errno, "Can't create trigger list");
  }
}

/*****************************************************************************/
C_confirmation* C_VED::GetConf(int xid, const struct timeval *timeout)
{
    ems_i32 *conf;
    msgheader header;
    const struct timeval* tout;
    int res;

    if (timeout)
        tout=timeout;
    else
        tout=communication.getdeftimeout();
    res=GetConfir(&xid, &ved, 0, &conf, &header, tout);
    if (res==-1)
        throw new C_ved_error(this, EMS_errno, "Can't get confirmation");
    if (communication.mcallback && messagepending())
        communication.mcallback(communication.mbackdata);
    if (res==0)
        throw new C_ved_error(this, ETIMEDOUT, "Can't get confirmation");
    return(new C_confirmation(conf, header, free_confirmation));
}
/*****************************************************************************/

void C_VED::ResetVED()
{
int xid;
xid=::ResetVED(ved);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in ResetVED");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::CreateVariable(int idx, int size)
{
int xid;
xid=::CreateVariable(ved, idx, size);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in CreateVariable");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::DeleteVariable(int idx)
{
int xid;
xid=::DeleteVariable(ved, idx);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DeleteVariable");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/
void C_VED::WriteVariable(int idx, ems_i32 val)
{
    int xid;
    xid=::WriteVariable(ved, idx, 1, (ems_u32*)&val);
    if (confmode_==synchron) {
        C_confirmation* conf;
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in WriteVariable");
        delete conf;
    } else {
        last_xid_=xid;
    }
}
/*****************************************************************************/
void C_VED::WriteVariable(int idx, ems_i32* source, int size)
{
    int xid;
    xid=::WriteVariable(ved, idx, size, (ems_u32*)source);
    if (confmode_==synchron) {
        C_confirmation* conf;
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in WriteVariable");
        delete conf;
    } else {
        last_xid_=xid;
    }
}
/*****************************************************************************/
int C_VED::ReadVariable(int idx)
{
    int xid, res;
    xid=::ReadVariable(ved, idx);
    if (confmode_==synchron) {
        C_confirmation* conf;
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK) {
            throw new C_ved_error(this, conf, "Error in ReadVariable");
        } else {
            if (conf->buffer(1)!=1) {
                OSTRINGSTREAM s;
                s << "Error in ReadVariable: expected size: 1; real size: "
                    << conf->buffer(1);
                delete conf;
                throw new C_ved_error(this, s);
            } else {
                res=conf->buffer(2);
                delete conf;
                return(res);
            }
        }
    } else {
        last_xid_=xid;
        return 0;
    }
}
/*****************************************************************************/

void C_VED::ReadVariable(int idx, ems_i32* dest, int* size)
{
    int xid;
    xid=::ReadVariable(ved, idx);
    if (confmode_==synchron) {
        C_confirmation* conf;
        ems_i32* buff;
        conf=GetConf(xid);
        buff=conf->buffer();
        if (buff[0]!=OK) {
            throw new C_ved_error(this, conf, "Error in ReadVariable");
        } else {
            int num;
            num=buff[1];
            if (num>*size) {
                OSTRINGSTREAM s;
                s << "Error in ReadVariable: available size: " << *size
                    << "; required size: " << num;
                delete conf;
                throw new C_ved_error(this, s);
            } else {
                int i;
                for (i=0; i<num; i++) dest[i]=buff[i+2];
                *size=num;
                delete conf;
            }
        }
    } else {
        last_xid_=xid;
    }
}
/*****************************************************************************/

void C_VED::ReadVariable(int idx, ems_i32** dest, int* size)
{
    int xid;
    xid=::ReadVariable(ved, idx);
    if (confmode_==synchron) {
        C_confirmation* conf;
        ems_i32* buff;
        conf=GetConf(xid);
        buff=conf->buffer();
        if (buff[0]!=OK) {
            throw new C_ved_error(this, conf, "Error in ReadVariable");
        } else {
            int num;
            num=buff[1];
            ems_i32* arr=new ems_i32[num];
            if (arr==0) {delete conf; throw new C_ved_error(this, errno);}
            for (int i=0; i<num; i++) arr[i]=buff[i+2];
            *size=num;
            *dest=arr;
            delete conf;
        }
    } else {
        last_xid_=xid;
    }
}

/*****************************************************************************/

int C_VED::GetVariableAttributes(int idx)
{
int xid, res;
xid=::GetVariableAttributes(ved, idx);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
    throw new C_ved_error(this, conf, "Error in GetVariableAttributes");
  else
    {
    res=conf->buffer(1);
    delete conf;
    return res;
    }
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

void C_VED::StartReadout()
{
int xid;
xid=::StartProgramInvocation(ved, Invocation_readout, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in StartReadout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::StopReadout()
{
int xid;
xid=::StopProgramInvocation(ved, Invocation_readout, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in StopReadout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::ResetReadout()
{
int xid;
xid=::ResetProgramInvocation(ved, Invocation_readout, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in ResetReadout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::ResumeReadout()
{
int xid;
xid=::ResumeProgramInvocation(ved, Invocation_readout, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in ResumeReadout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

InvocStatus C_VED::GetReadoutStatus(ems_u32* eventcount)
{
int xid;
xid=::GetProgramInvocationAttributes(ved, Invocation_readout, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  InvocStatus status;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetReadoutStatus");
  status=(InvocStatus)conf->buffer(1);
  if (eventcount) *eventcount=conf->buffer(2);
  delete conf;
  return(status);
  }
else
  {
  last_xid_=xid;
  return (InvocStatus)0;
  }
}

/*****************************************************************************/

C_readoutstatus* C_VED::GetReadoutStatus(int naux, const int* aux, int useaux)
{
int xid;
ems_u32* body;

body=new ems_u32[2+naux];
if (body==0)
    throw new C_unix_error(errno, "alloc request for GetReadoutStatus");
body[0]=Invocation_readout;
body[1]=0;
for (int i=0; i<naux; i++) body[2+i]=aux[i];

xid=::Rawrequest(ved, Req_GetProgramInvocationAttr, 2+naux, body);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetReadoutStatus");
  C_inbuf inbuf(conf->buffer(), conf->size());
  delete conf;
  C_readoutstatus* status=new C_readoutstatus(inbuf, useaux);
  return status;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

STRING C_VED::GetReadoutParams()
{
int xid;

xid=::GetProgramInvocationParams(ved, Invocation_readout, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  STRING st;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetReadoutParams");
  if (conf->size()<2) return "not known";
  C_inbuf ib(conf->buffer(), conf->size());
  delete conf;
  ib++;
  ib >> st;
  return st;
  }
else
  {
  last_xid_=xid;
  return "";
  }
}

/*****************************************************************************/

C_data_io* C_VED::UploadDataout(int idx)
{
C_confirmation* conf;
int xid;
C_data_io* addr;

if ((xid=UploadDomain(ved, Dom_Dataout, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't upload dataout");
if (confmode_==synchron)
  {
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
    {
    throw new C_ved_error(this, conf, "Error in UploadDataout");
    }

  addr=C_data_io::create(io_out, conf);
  delete conf;
  return addr;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

void C_VED::UploadDataoutAddr(int idx, C_add_trans& addr)
{
    C_confirmation* conf;
    int xid, address;
    ems_i32 *buff;

    if ((xid=UploadDomain(ved, Dom_Dataout, idx))==-1)
        throw new C_ved_error(this, EMS_errno, "Can't upload dataoutaddr");
    if (confmode_==synchron) {
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in UploadDataoutAddr");

      /*
        conf[0]: Error;
        conf[1]: bufftyp;
        conf[2]: buffsize;
        conf[3]: Prioritaet;
        conf[4]: addrtype;
        switch (addrtype) {
        case Addr_Modul:
            conf[5]: modulname;
            conf[?]: addr, falls OSK;
        case Addr_Raw:
            conf[5]: addr;
        case Addr_Socket:
        case Addr_LocalSocket:
        case Addr_File:
        Addr_Tape:
        Addr_Null:
            nicht brauchbar;
      */
        buff=conf->buffer();
        switch (buff[4]) {
        case Addr_Raw:
            address=buff[5];
            break;
        case Addr_Modul:
            {
            ems_i32* help;
            help=buff+5+xdrstrlen(buff+5);
            if (help-buff>=conf->size())
                throw new C_ved_error(this, "addresstype is shared memory");
            address=*help;
            }
            break;
        case Addr_Socket:
        case Addr_LocalSocket:
        case Addr_File:
        case Addr_Tape:
        case Addr_Null:
            {
            OSTRINGSTREAM s;
            s << "addresstype " << buff[4] << " is not usable";
            throw new C_ved_error(this, s);
            }
        default:
            {
            OSTRINGSTREAM s;
            s << "addresstype "<< buff[4] <<" is not known";
            throw new C_ved_error(this, s);
            }
        }
        delete conf;
        addr.setoffs(address);
    } else {
        last_xid_=xid;
    }
}
/*****************************************************************************/

int C_VED::SetUnsol(int val)
{
return(::SetUnsolicited(ved, val));
}

/*****************************************************************************/

C_confirmation* C_VED::Initiate(int id)
{
int xid;
xid=::Initiate(ved, id);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in Initiate");
  return conf;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

C_confirmation* C_VED::GetVEDStatus(void)
{
int xid;
xid=::GetVEDStatus(ved);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetVEDStatus");
  return conf;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

C_confirmation* C_VED::Identify(int level)
{
int xid;
xid=::Identify(ved, level);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in Identify");
  return conf;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

C_confirmation* C_VED::UploadEvent()
{
int xid;
xid=::UploadDomain(ved, Dom_Event, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in UploadEvent");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_confirmation* C_VED::GetNamelist()
{
int xid;
xid=::GetNameList(ved, 0, 0);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetNamelist");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_confirmation* C_VED::GetNamelist(Object obj)
{
int xid;
int list[1];
list[0]=obj;
xid=::GetNameList(ved, 1, list);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetNamelist");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_confirmation* C_VED::GetNamelist(Object obj, Domain dom)
{
int xid;
int list[2];
list[0]=obj;
list[1]=dom;
xid=::GetNameList(ved, 2, list);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetNamelist");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_confirmation* C_VED::GetNamelist(Object obj, Invocation inv)
{
int xid;
int list[2];
list[0]=obj;
list[1]=inv;
xid=::GetNameList(ved, 2, list);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetNamelist");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_confirmation* C_VED::GetNamelist(int* ptr, int size)
{
int xid;
xid=::GetNameList(ved, size, ptr);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetNamelist");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

C_confirmation* C_VED::GetNamelist(int num, ...)
{
va_list vl;
int xid, i;
int* list=new int[num];
va_start(vl, num);
va_arg(vl, int); // num ueberspringen
for (i=0; i<num; i++) list[i]=va_arg(vl, int);
va_end(vl);
xid=::GetNameList(ved, num, list);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetNamelist");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

void C_VED::create_is(int idx, int id)
{
int xid;
xid=::CreateIS(ved, idx, id);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in create_is");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

conf_mode C_VED::confmode(conf_mode mode)
{
conf_mode old=confmode_;
confmode_=mode;
return old;
}

/*****************************************************************************/
int C_VED::loadcaplist(Capabtyp typ, C_capability_list** capab_list)
{
    const ems_i32 *p;
    int num, xid, res, i;
    C_confirmation* conf;

    if ((xid=::GetCapabilityList(ved, typ))==-1)
        throw new C_ved_error((C_VED*)0, EMS_errno,
            "Can't request list of capabilities");
    conf=GetConf(xid);
    if ((res=conf->buffer(0))!=OK) {
        delete conf;
        return res;
    }
    p=(conf->buffer())+1;
    num=*(p++);
    try {
        *capab_list=new C_capability_list(num);
    } catch(C_error*) {
        delete conf;
        throw;
    }
    for (i=0; i<num; i++) {
        int idx, ver;
        char* s;
        idx=*(p++);
        s=new char[*p+1];
        if (s==0) {
            delete conf;
            delete *capab_list;
            *capab_list=0;
            throw new C_ved_error((C_VED*)0, errno,
                "Can't store capability list");
        }
        p=(ems_i32*)extractstring(s, (ems_u32*)p);
        ver=*(p++);
        (*capab_list)->add(s, idx, ver);
    }
    delete conf;
    return OK;
}
/*****************************************************************************/
void C_VED::add_is(C_instr_system* is)
{
    C_instr_system** help=new C_instr_system*[num_is+1];
    for (int i=0; i<num_is; i++) help[i]=islist[i];
    delete[] islist;
    islist=help;
    islist[num_is]=is;
    num_is++;
}
/*****************************************************************************/
void C_VED::delete_is(C_instr_system* is)
{
    int i;
    for (i=0; (i<num_is) && (islist[i]!=is); i++);
    if (i==num_is) {
        cerr<<"C_VED::delete_is: is "<<(void*)is<<" not found"<<endl;
        return;
    }
    for (int j=i; j<num_is-1; j++)
        islist[j]=islist[j+1];
    num_is--;
}
/*****************************************************************************/

int C_VED::procnum(const char* proc, Capabtyp typ)
{
C_capability_list* list=typ==Capab_listproc?capab_proc_list:capab_trig_list;
if (list)
  {
  int res;
  if ((res=list->get(proc))!=-1)
    return(res);
  else
    {
    OSTRINGSTREAM s;
    s<<"VED has no "<<(typ==Capab_listproc?"":"trigger ")
        <<"procedure "<< proc;
    throw new C_ved_error(this, s);
    }
  }
else
  {
  OSTRINGSTREAM s;
  s << "VED has no "<<(typ==Capab_listproc?"":"trigger ")
      <<"procedures";
  throw new C_ved_error(this, s);
  }
}

/*****************************************************************************/

int C_VED::numprocs(Capabtyp typ)
{
C_capability_list* list=typ==Capab_listproc?capab_proc_list:capab_trig_list;
if (list)
  return(list->size());
else
  return(0);
}

/*****************************************************************************/

STRING C_VED::procname(int proc, Capabtyp typ)
{
C_capability_list* list=typ==Capab_listproc?capab_proc_list:capab_trig_list;
if (list)
  return(list->get(proc));
else
  {
  OSTRINGSTREAM s;
  s << "VED has no "<<(typ==Capab_listproc?"":"trigger ")
      <<"procedures";
  throw new C_ved_error(this, s);
  }
}

/*****************************************************************************/

char C_VED::version_separator(char c, Capabtyp typ)
{
C_capability_list* list=typ==Capab_listproc?capab_proc_list:capab_trig_list;
char res=-1;

if (list)
  res=list->version_separator(c);
return(res);
}

/*****************************************************************************/

char C_VED::version_separator(Capabtyp typ)
{
C_capability_list* list=typ==Capab_listproc?capab_proc_list:capab_trig_list;
if (list)
  return(list->version_separator());
else
  return(-1);
}

/*****************************************************************************/

C_confirmation* C_VED::GetProcProperties(int level, int num, ems_u32* list)
{
int xid;
xid=::GetProcProperties(ved, level, num, list);
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetProcProperties");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

void C_VED::downdatain(int idx, int size, const ems_u32* domain)
{
int xid;

if ((xid=::DownloadDomain(ved, Dom_Datain, idx, size, domain))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't download datain");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DownloadDatain");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::DownloadDatain(int idx, const C_data_io& addr)
{
//addr.pruefen();
C_outbuf b;
b << addr;
downdatain(idx, b.size(), b.buf());
}

/*****************************************************************************/

// nur fuer addrtyp==Addr_Raw brauchbar
void C_VED::DownloadDatain(int idx, InOutTyp typ, IOAddr addrtyp,
    C_add_trans addr)
{
ems_u32 domain[3];

if (addrtyp!=Addr_Raw)
    throw new C_ved_error(this,
      "addrtyp in DownloadDatain() must be \"Addr_Raw\"");

domain[0]=typ;
domain[1]=addrtyp;
domain[2]=addr;

downdatain(idx, 3, domain);
}
/*****************************************************************************/

// Modul, LocalSocket
void C_VED::DownloadDatain(int idx, InOutTyp typ, IOAddr addrtyp,
    const char* name)
{
ems_u32 *domain;
int len;

if ((addrtyp!=Addr_Modul) && (addrtyp!=Addr_LocalSocket))
    throw new C_ved_error(this,
      "addrtyp in DownloadDatain() must be \"Addr_Modul\" "
      "or \"Addr_LocalSocket\"");

len=strxdrlen(name);
len+=2;
domain=new ems_u32[len];
domain[0]=typ;
domain[1]=addrtyp;
outstring(domain+2, name);

try
  {
  downdatain(idx, len, domain);
  }
catch(C_error*)
  {
  delete[] domain;
  throw;
  }
delete[] domain;
}
/*****************************************************************************/

// Driver_mapped, Driver_mixed, Driver_syscall
void C_VED::DownloadDatain(int idx, InOutTyp typ, IOAddr addrtyp,
    const char* name, int space, C_add_trans offset, int option)
{
ems_u32 *domain;
int len;

if ((addrtyp!=Addr_Driver_mapped) && (addrtyp!=Addr_Driver_mixed) &&
    (addrtyp!=Addr_Driver_syscall))
    throw new C_ved_error(this,
      "addrtyp in DownloadDatain() must be "
      "or \"Addr_Driver_mapped\" "
      "or \"Addr_Driver_mixed\" "
      "or \"Addr_Driver_syscall\"");

len=strxdrlen(name);
ems_u32* help;
len+=5;
domain=new ems_u32[len];
domain[0]=typ;
domain[1]=addrtyp;
help=outstring(domain+2, name);
help[0]=space;
help[1]=offset;
help[2]=option;

try
  {
  downdatain(idx, len, domain);
  }
catch(C_error*)
  {
  delete[] domain;
  throw;
  }
delete[] domain;
}
/*****************************************************************************/

void C_VED::DeleteDatain(int id)
{
int xid;

if ((xid=::DeleteDomain(ved, Dom_Datain, id))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't delete datain");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DeleteDatain");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

C_data_io* C_VED::UploadDatain(int idx)
{
C_confirmation* conf;
int xid;
C_data_io* addr;

if ((xid=::UploadDomain(ved, Dom_Datain, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't upload datain");
if (confmode_==synchron)
  {
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in UploadDatain");

  addr=C_data_io::create(io_in, conf);
  delete conf;
  return addr;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

void C_VED::downdataout(int idx, int size, const ems_u32* domain)
{
int xid;

if ((xid=::DownloadDomain(ved, Dom_Dataout, idx, size, domain))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't download dataout");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DownloadDataout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::DownloadDataout(int idx, const C_data_io& addr)
{
C_outbuf b;
b << addr;
downdataout(idx, b.index(), b.buf());
}

/*****************************************************************************/

// nur fuer addrtyp==Addr_Null brauchbar
void C_VED::DownloadDataout(int idx, InOutTyp typ, int buffersize,
    int priority, IOAddr addrtyp)
{
ems_u32 domain[4];

if (addrtyp!=Addr_Null)
    throw new C_ved_error(this,
      "addrtyp in DownloadDataout must be \"Addr_Null\"");

domain[0]=typ;
domain[1]=buffersize;
domain[2]=priority;
domain[3]=addrtyp;

downdataout(idx, 4, domain);
}
/*****************************************************************************/

// nur fuer addrtyp==Addr_Raw brauchbar
void C_VED::DownloadDataout(int idx, InOutTyp typ, int buffersize,
    int priority, IOAddr addrtyp, int addr)
{
ems_u32 domain[5];

if (addrtyp!=Addr_Raw)
    throw new C_ved_error(this,
      "addrtyp in DownloadDataout must be \"Addr_Raw\"");

domain[0]=typ;
domain[1]=buffersize;
domain[2]=priority;
domain[3]=addrtyp;
domain[4]=addr;

downdataout(idx, 5, domain);
}
/*****************************************************************************/

// Modul, LocalSocket, Tape, File
void C_VED::DownloadDataout(int idx, InOutTyp typ, int buffersize,
    int priority, IOAddr addrtyp, const char* name)
{
ems_u32 *domain;
int len;

if ((addrtyp!=Addr_Modul) && (addrtyp!=Addr_LocalSocket) &&
    (addrtyp!=Addr_Tape) && (addrtyp!=Addr_File))
  throw new C_ved_error(this,
    "addrtyp in DownloadDataout() must be \"Addr_Modul\" "
    "or \"Addr_LocalSocket\" "
    "or \"Addr_Tape\" "
    "or \"Addr_File\"");

len=strxdrlen(name);
domain=new ems_u32[4+len];
domain[0]=typ;
domain[1]=buffersize;
domain[2]=priority;
domain[3]=addrtyp;
outstring(domain+4, name);

try
  {
  downdataout(idx, 4+len, domain);
  }
catch(C_error*)
  {
  delete[] domain;
  throw;
  }
delete[] domain;
}
/*****************************************************************************/

// Addr_Socket
void C_VED::DownloadDataout(int idx, InOutTyp typ, int buffersize,
    int priority, IOAddr addrtyp, const char* hostname, int port)
{
char hname[256];
ems_u32 domain[6];
struct hostent* host;
int addr;

if (addrtyp!=Addr_Socket)
    throw new C_ved_error(this,
      "addrtyp in DownloadDataout must be \"Addr_Socket\"");

if ((hostname==0) || (strcmp(hostname, "")==0))
  gethostname(hname, 256);
else
  strcpy(hname, hostname);
host=gethostbyname(hname);
if (host!=0)
  addr= *(int*)(host->h_addr_list[0]);
else
  {
  addr=inet_addr(hname);
  if (addr==-1)
    {
    OSTRINGSTREAM s;
    s << "DownloadDataout: host \""<< hname << "\" unknown";
    throw new C_ved_error(this, EMSE_HostUnknown, s);
    }
  }

domain[0]=typ;
domain[1]=buffersize;
domain[2]=priority;
domain[3]=addrtyp;
domain[4]=ntohl(addr);
domain[5]=port;
downdataout(idx, 6, domain);
}
/*****************************************************************************/

void C_VED::DeleteDataout(int id)
{
int xid;

if ((xid=DeleteDomain(ved, Dom_Dataout, id))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't delete dataout");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DeleteDataout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

C_confirmation* C_VED::GetDataoutStatus(int idx, int arg)
{
int xid;

if (arg)
  {
  if ((xid=::GetDataoutStatus_1(ved, idx, arg))==-1)
      throw new C_ved_error(this, EMS_errno, "Can't get dataout status (1)");
  }
else
  {
  if ((xid=::GetDataoutStatus(ved, idx))==-1)
      throw new C_ved_error(this, EMS_errno, "Can't get dataout status");
  }

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in GetDataoutStatus");
  return conf;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

void C_VED::WindDataout(int id, int offset)
{
int xid;

if ((xid=::WindDataout(ved, id, offset))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't wind dataout");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in WindDataout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::WriteDataout(int id, int default_header, int size, const ems_u32* data)
{
int xid;

if ((xid=::WriteDataout(ved, id, default_header, size, data))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't write dataout");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in WriteDataout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::WriteDataout(int id, int default_header, const C_buf& ib)
{
WriteDataout(id, default_header, ib.size(), ib.buf());
}

/*****************************************************************************/
void C_VED::WriteDataout(int id, int size, const char* s)
{
    C_outbuf ob;
    const char *p1, *p2;
    int x, m, n, l;
    ob << space_(x);
    p1=s; n=size; m=0;
    while (n) {
        p2=(const char*)memchr(p1, '\n', n);
        if (p2) {
            l=p2-p1;
            ob<<put(p1, l);
            p1+=l+1;
            n-=l+1;
        } else {
            ob<<p1;
            n=0;
        }
        m++;
    }
    ob<<put(x, m);
    WriteDataout(id, clusterty_text, ob.size(), ob.buf());
}
/*****************************************************************************/
void C_VED::WriteDataout(int id, string st)
{
    WriteDataout(id, st.length(), st.c_str());
}
/*****************************************************************************/

void C_VED::EnableDataout(int id, int val)
{
int xid;

if (val)
  xid=::EnableDataout(ved, id);
else
  xid=::DisableDataout(ved, id);
if (xid==-1)
    throw new C_ved_error(this, EMS_errno, "Can't enable dataout");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in EnableDataout");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::DeleteIS(int idx)
{
int xid;

if ((xid=::DeleteIS(ved, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send DeleteIs");
if (confmode_==synchron)
  {
  C_confirmation* conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
    {
    OSTRINGSTREAM s;
    s << "Can't delete IS " << idx;
    throw new C_ved_error(this, conf, s);
    }
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/
C_isstatus* C_VED::ISStatus(int idx)
{
    int xid;

    if ((xid=::GetISStatus(ved, idx))==-1)
        throw new C_ved_error(this, EMS_errno, "Can't send GetISStatus");
    if (confmode_==synchron) {
        C_confirmation* conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in GetISStatus");
        const ems_i32* b=conf->buffer();
        C_isstatus* status=new C_isstatus(b[1], b[2], b[3], b[4],
                (const ems_u32*)(b+5));
        delete conf;
        return status;
    } else {
        last_xid_=xid;
        return 0;
    }
}
/*****************************************************************************/
void
C_VED::CreateLam(int idx, int id, int is, const char* trigproc,
    const int* args, int argnum)
{
    int iproc;

    if (!capab_trig_list)
        throw new C_ved_error(this, "No trigger procedures available");
    iproc=capab_trig_list->get(trigproc);
    if (iproc==-1) {
        OSTRINGSTREAM s;
        s << "unknown trigger procedure \"" << trigproc << "\"";
        throw new C_ved_error(this, s);
    }

    ems_u32* nargs=new ems_u32[argnum+4];
    nargs[0]=id;
    nargs[1]=is;
    nargs[2]=iproc;
    nargs[3]=argnum;
    for (int i=0; i<argnum; i++)
        nargs[4+i]=args[i];

    ems_i32 xid;
    xid=::CreateProgramInvocation(ved, Invocation_LAM, idx, argnum+4, nargs);
    delete[] nargs;
    if (xid==-1)
        throw new C_ved_error(this, EMS_errno, "Can't send CreateLam");
    if (confmode_==synchron) {
        C_confirmation* conf;
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in CreateLam");
        delete conf;
    } else {
        last_xid_=xid;
    }
}
/*****************************************************************************/

C_confirmation* C_VED::UploadLam(int idx)
{
int xid;

if ((xid=::GetProgramInvocationAttributes(ved, Invocation_LAM, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send UploadLam");
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in CreateLam");
  return(conf);
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

void C_VED::DeleteLam(int idx)
{
int xid;

if ((xid=::DeleteProgramInvocation(ved, Invocation_LAM, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send DeleteLam");
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DeleteLam");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::StartLam(int idx)
{
int xid;

if ((xid=::StartProgramInvocation(ved, Invocation_LAM, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send StartLam");
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in StartLam");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::ResetLam(int idx)
{
int xid;

if ((xid=::ResetProgramInvocation(ved, Invocation_LAM, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send ResetLam");
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in ResetLam");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::StopLam(int idx)
{
int xid;

if ((xid=::StopProgramInvocation(ved, Invocation_LAM, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send StopLam");
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in StopLam");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::ResumeLam(int idx)
{
int xid;

if ((xid=::ResumeProgramInvocation(ved, Invocation_LAM, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send ResumeLam");
if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in ResumeLam");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/
#if 0
void
C_VED::DownloadModullist(int modules, int* list)
{
    int xid, i;
    ems_u32* liste;

    liste=new ems_u32[modules*2+1];
    liste[0]=modules;
    for (i=0; i<modules*2; i++)
        liste[i+1]=list[i];

    xid=::DownloadDomain(ved, Dom_Modullist, 0, modules*2+1, liste);
    delete[] liste;

    if (xid==-1)
        throw new C_ved_error(this, EMS_errno, "Can't send modullist");
    if (confmode_==synchron) {
        C_confirmation* conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in DownloadModullist");
        delete conf;
    } else {
        last_xid_=xid;
    }
}
#endif
/*****************************************************************************/
void
C_VED::DownloadModullist(const C_modullist& mlist, int version)
{
    C_outbuf b;
    int i;

    if (version==0) {
        for (i=0; i<mlist.size(); i++) {
            if (mlist.modulclass(i)!=modul_unspec) {
                throw new C_program_error(
                    "DownloadModullist: old version but modulclass!=unspec"
                    );
            }
        }
    }

    switch (version) {
    case 0:
        b<<mlist.size();
        for (i=0; i<mlist.size(); i++) {
            const C_modullist::ml_entry& entry=mlist.get(i);
            b<<entry.address.unspec.addr;
            b<<entry.modultype;
        }
        break;
    case 1:
        b<<mlist.size();
        for (i=0; i<mlist.size(); i++) {
            const C_modullist::ml_entry& entry=mlist.get(i);
            b<<entry.modultype;
            b<<entry.modulclass;
            switch (entry.modulclass) {
            case modul_none:
                break;
            case modul_unspec:
                b<<entry.address.unspec.addr;
                break;
            case modul_generic:
                throw new C_program_error("C_VED::DownloadModullist: "
                        "modul_generic not allowed");
                break;
            case modul_camac:
            case modul_fastbus:
            case modul_vme:
            case modul_lvd:
            case modul_can:
            case modul_perf:
            case modul_caenet:
            case modul_sync:
                b<<entry.address.adr2.crate;
                b<<entry.address.adr2.addr;
                break;
            case modul_pcihl:
                throw new C_program_error("C_VED::DownloadModullist: "
                        "modul_pcihl not allowed");
            case modul_invalid:
                throw new C_program_error("C_VED::DownloadModullist: "
                        "modul_invalid not allowed");
            }
        }
        break;
    default:
        throw new C_program_error("DownloadModullist: illegal version");
    }

    /* Domain-ID is missused as version number */
    ems_i32 xid=::DownloadDomain(ved, Dom_Modullist, version, b.size(), b.buf());

    if (xid==-1)
        throw new C_ved_error(this, EMS_errno, "Can't send modullist");
    if (confmode_==synchron) {
        C_confirmation* conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in DownloadModullist");
        delete conf;
    } else {
        last_xid_=xid;
    }
}
/*****************************************************************************/

C_modullist* C_VED::UploadModullist(int version)
{
int xid;

if ((xid=::UploadDomain(ved, Dom_Modullist, version))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send upload_modullist");
if (confmode_==synchron)
  {
  C_confirmation* conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in UploadModullist");


  C_modullist* modullist=new C_modullist(conf, version);
  return(modullist);
  }
else
  {
  last_xid_=xid;
  return(0);
  }
}

/*****************************************************************************/

void C_VED::DeleteModullist()
{
int xid;

if ((xid=::DeleteDomain(ved, Dom_Modullist, 0))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send delete_modullist");
if (confmode_==synchron)
  {
  C_confirmation* conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DeleteModullist");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::lam_add_param(int num, int par, ...)
{
va_list vl;
int i;

lamlist->add_par(par);
va_start(vl, par);
for (i=0; i<num-1; i++) lamlist->add_par(va_arg(vl, int));
va_end(vl);
}

/*****************************************************************************/

void C_VED::lam_add_proc(const char* proc, int argnum, ...)
{
va_list vl;
int i;

if (lamlist->add_proc(proc)==-1)
  {
  OSTRINGSTREAM s;
  s << "unknown procedure \"" << proc << "\"";
  throw new C_ved_error(this, s);
  }

va_start(vl, argnum);
for (i=0; i<argnum; i++) lamlist->add_par(va_arg(vl, int));
va_end(vl);
}

/*****************************************************************************/

void C_VED::lam_add_proc(const char* proc, int arg)
{
if (lamlist->add_proc(proc)==-1)
  {
  OSTRINGSTREAM s;
  s << "unknown procedure \"" << proc << "\"";
  throw new C_ved_error(this, s);
  }

lamlist->add_par(arg);
}

/*****************************************************************************/

void C_VED::lam_add_proc(const char* proc)
{
if (lamlist->add_proc(proc)==-1)
  {
  OSTRINGSTREAM s;
  s << "unknown procedure \"" << proc << "\"";
  throw new C_ved_error(this, s);
  }
}

/*****************************************************************************/
void
C_VED::DownloadLamproc(int idx, int unsolflag)
{
    int xid, size;
    ems_u32 *list;

    lamlist->getlist(&list, &size);
    ems_u32* dom=new ems_u32[size+1];
    dom[0]=unsolflag;
    for (int i=0; i<size; i++)
        dom[i+1]=list[i];
    xid=::DownloadDomain(ved, Dom_LAMproclist, idx, size+1, dom);
    delete[] dom;
    if (xid==-1)
        throw new C_ved_error(this, EMS_errno, "Can't send DownloadLamproc");

    if (confmode_==synchron) {
        C_confirmation* conf;
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in DownloadLamproc");
        delete conf;
    } else {
        last_xid_=xid;
    }
}
/*****************************************************************************/
C_confirmation*
C_VED::UploadLamproc(int idx)
{
    int xid;
    if ((xid=::UploadDomain(ved, Dom_LAMproclist, idx))==-1)
        throw new C_ved_error(this, EMS_errno, "Can't send DownloadLamproc");

    if (confmode_==synchron) {
        C_confirmation* conf;
        conf=GetConf(xid);
        if (conf->buffer(0)!=OK)
            throw new C_ved_error(this, conf, "Error in UploadLamproc");
        return conf;
    } else {
        last_xid_=xid;
        return 0;
    }
}
/*****************************************************************************/

void C_VED::DeleteLamproc(int idx)
{
int xid;
if ((xid=::DeleteDomain(ved, Dom_LAMproclist, idx))==-1)
    throw new C_ved_error(this, EMS_errno, "Can't send DeleteLamproc");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DeleteLamproc");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::trig_set_proc(const char* proc)
{
if (triglist->proc_num()>0)
  {
  throw new C_ved_error(this, "only one procedure is allowed in trigglist");
  }
if (triglist->add_proc(proc)==-1)
  {
  OSTRINGSTREAM s;
  s << "unknown trigger procedure \"" << proc << "\"";
  throw new C_ved_error(this, s);
  }
}

/*****************************************************************************/

void C_VED::trig_set_proc(int proc)
{
if (triglist->proc_num()>0)
  {
  throw new C_ved_error(this, "only one procedure is allowed in trigglist");
  }
triglist->add_proc(proc);
}

/*****************************************************************************/

void C_VED::trig_add_param(int num, int par, ...)
{
va_list vl;
int i;

triglist->add_par(par);
va_start(vl, par);
for (i=0; i<num-1; i++) triglist->add_par(va_arg(vl, int));
va_end(vl);
}

/*****************************************************************************/

void C_VED::DownloadTrigger(const char* proc, int argnum, ...)
{
va_list vl;
int i, iproc, xid;
ems_u32* domain;

if (!capab_trig_list)
    throw new C_ved_error(this, "No trigger procedures available");
iproc=capab_trig_list->get(proc);
if (iproc==-1)
  {
  OSTRINGSTREAM s;
  s << "unknown trigger procedure \"" << proc << "\"";
  throw new C_ved_error(this, s);
  }

domain=new ems_u32[argnum+2];

domain[0]=iproc;
domain[1]=argnum;

va_start(vl, argnum);
for (i=0; i<argnum; i++) domain[i+2]=va_arg(vl, int);
va_end(vl);

xid=DownloadDomain(ved, Dom_Trigger, 0, argnum+2, domain);
delete[] domain;

if (xid==-1)
    throw new C_ved_error(this, EMS_errno, "Can't download trigger procedure");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DownloadTrigger");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::DownloadTrigger(const char* proc, int* args,
    int argnum)
{
int i, iproc, xid;
ems_u32* domain;

if (!capab_trig_list)
    throw new C_ved_error(this, "No trigger procedures available");
iproc=capab_trig_list->get(proc);
if (iproc==-1)
  {
  OSTRINGSTREAM s;
  s << "unknown trigger procedure \"" << proc << "\"";
  throw new C_ved_error(this, s);
  }

domain=new ems_u32[argnum+2];

domain[0]=iproc;
domain[1]=argnum;

for (i=0; i<argnum; i++) domain[i+2]=args[i];

xid=DownloadDomain(ved, Dom_Trigger, 0, argnum+2, domain);
delete[] domain;

if (xid==-1)
    throw new C_ved_error(this, EMS_errno, "Can't download trigger procedure");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DownloadTrigger");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

void C_VED::DownloadTrigger()
{
int xid;
ems_u32* list;
int listsize;

triglist->getlist(&list, &listsize);

xid=DownloadDomain(ved, Dom_Trigger, 0, listsize-1, list+1);
triglist->clear();
if (xid==-1)
    throw new C_ved_error(this, EMS_errno, "Can't download trigger procedure");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DownloadTrigger");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/

C_confirmation* C_VED::UploadTrigger()
{
int xid;

xid=UploadDomain(ved, Dom_Trigger, 0);

if (xid==-1)
    throw new C_ved_error(this, EMS_errno, "Can't upload trigger procedure");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in UploadTrigger");
  return conf;
  }
else
  {
  last_xid_=xid;
  return 0;
  }
}

/*****************************************************************************/

void C_VED::DeleteTrigger()
{
int xid;

xid=DeleteDomain(ved, Dom_Trigger, 0);

if (xid==-1)
    throw new C_ved_error(this, EMS_errno, "Can't delete trigger procedure");

if (confmode_==synchron)
  {
  C_confirmation* conf;
  conf=GetConf(xid);
  if (conf->buffer(0)!=OK)
      throw new C_ved_error(this, conf, "Error in DeleteTrigger");
  delete conf;
  }
else
  last_xid_=xid;
}

/*****************************************************************************/
/*****************************************************************************/
