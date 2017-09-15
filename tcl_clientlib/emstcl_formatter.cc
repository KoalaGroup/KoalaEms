/*
 * emstcl_formatter.cc
 * created 29.11.96
 * 22.Jan.2001 PW: multicrate support (f_modullist)
 */
#include "config.h"
#include "cxxcompat.hxx"
#include <stdlib.h>
#include "emstcl_ved.hxx"
#include "emstcl_is.hxx"
#include <proc_dataoutstatus.hxx>
#include <errors.hxx>
#include <conststrings.h>
#include "findstring.hxx"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl_formatter.cc,v 1.22 2010/02/03 00:15:50 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

int E_ved::f_void(OSTRINGSTREAM& ss, const C_confirmation* conf, int needtest,
    int numargs, const STRING* args)
{
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_dummy(OSTRINGSTREAM& ss, const C_confirmation* conf, int needtest,
    int numars, const STRING* args)
{
int num=conf->size();
//int* buf=conf->buffer();
ss << num;
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_confirmation(OSTRINGSTREAM& ss, const C_confirmation* conf, int needtest,
    int numars, const STRING* args)
{
ss << E_confirmation::new_E_confirmation(interp, this, *conf);
return TCL_OK;
}

/*****************************************************************************/

// int E_ved::f_printconf(OSTRINGSTREAM& ss, const C_confirmation* conf,
//     int needtest, int numargs, const STRING* args)
// {
// if ((conf->size()>=1) && (conf->buffer(0)!=0))
//   {
//   C_confirmation* nconf=new C_confirmation(*conf);
//   C_ved_error* err=new C_ved_error(this, nconf);
//   ss << (*err);
//   delete err;
//   }
// else
//   {
//   Tcl_SetObjResult(interp, Tcl_NewStringObj("mangel in E_ved::f_printconf", -1));
//   return TCL_ERROR;
//   }
// //C_inbuf ib(conf->buffer(), conf->size());
// //ib++; // errorcode ueberspringen
// 
// return TCL_OK;
// }

/*****************************************************************************/
int E_ved::f_printconf_raw(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
int max;
if (numargs>0)
  {
  if (Tcl_GetInt(interp, (char*)args[0].c_str(), &max)!=TCL_OK)
      return TCL_ERROR;
  }
ss << '{' << (unsigned int)conf->header()->size;
ss << ' ' << (unsigned int)conf->header()->client;
ss << ' ' << (unsigned int)conf->header()->ved;
ss << ' ' << (unsigned int)conf->header()->type.reqtype;
ss << ' ' << (unsigned int)conf->header()->flags;
ss << ' ' << (unsigned int)conf->header()->transid << "} {"
    << hex << setiosflags(ios::showbase);

if (numargs>0)
  {if (max>(int)conf->header()->size) max=conf->header()->size;}
else
  max=conf->header()->size;

for (int i=0; i<max; i++)
  {
  if (i>0) ss << ' ';
  ss << (unsigned int)conf->buffer(i);
  }
if ((int)conf->header()->size>max)  ss << " ...";
ss << '}';

return TCL_OK;
}
/*****************************************************************************/
int E_ved::f_printconf_tabular(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
ss << (*conf);
return TCL_OK;
}
/*****************************************************************************/
int E_ved::f_printconf_text(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
ss<</*name()<<": "<<*/Req_str(conf->header()->type.reqtype)<<": ";
if (conf->buffer(0))
  {
  ss<<R_errstr((errcode)conf->buffer(0));
  }
else
  {
  ss<<"no error";
  }
return TCL_OK;
}
/*****************************************************************************/

int E_ved::f_command(OSTRINGSTREAM& ss, const C_confirmation* conf, int needtest,
    int numargs, const STRING* args)
{
int num=conf->size();
unsigned int* buf=(unsigned int*)(conf->buffer());
for (int i=1; i<num; i++) {ss << buf[i]; if (i+1<num) ss << ' ';}
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_readvar(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
ems_i32* buf=conf->buffer();
int size=buf[1];
int first=0, last=size;
if (numargs>0)
  {
  if (Tcl_GetInt(interp, (char*)args[0].c_str(), &first)!=TCL_OK)
      return TCL_ERROR;
  if (first>=last)
    {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("not enough elements in variable", -1));
    return TCL_ERROR;
    }
  }
if (numargs>1)
  {
  int num;
  if (Tcl_GetInt(interp, (char*)args[1].c_str(), &num)!=TCL_OK)
      return TCL_ERROR;
  if (first+num>=last)
    {
    Tcl_SetObjResult(interp, Tcl_NewStringObj("not enough elements in variable", -1));
    return TCL_ERROR;
    }
  last=first+num;
  }
for (int i=first; i<last; i++)
  {
  if (i>first) ss << ' ';
  ss << buf[i+2];
  }
ss << endl;
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_integer(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
if (conf->size()<2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("confirmation contains no data", -1));
  return TCL_ERROR;
  }
ss << conf->buffer(1);
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_integerlist(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
C_inbuf ib(conf->buffer(), conf->size());
ib++; // errorcode ueberspringen
int num=ib.getint();
for (int i=0; i<num; i++)
  {
  ss << ib.getint();
  if (i+1<num) ss << ' ';
  }
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_rawintegerlist(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
C_inbuf ib(conf->buffer(), conf->size());
ib++; // errorcode ueberspringen
int num=conf->size()-1;
if (numargs>0)
  {
  int n;
  if (Tcl_GetInt(interp, (char*)args[0].c_str(), &n)!=TCL_OK) return TCL_ERROR;
  if (n<num) num=n;
  }
for (int i=0; i<num; i++)
  {
  // ss << hex << setiosflags(ios::showbase);
  ss << ib.getint();
  if (i+1<num) ss << ' ';
  }
return TCL_OK;
}

/*****************************************************************************/
int E_ved::f_intfmt(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
    C_inbuf ib(conf->buffer(), conf->size());
    const char* format="0x%08x";
    char s[32];
    int n, num, first, last;

    ib++; // errorcode ueberspringen
    n=num=conf->size()-1;
    first=0;

    if (numargs>0) {
        format=(char*)args[0].c_str();
    }
    if (numargs>2) {
        if (Tcl_GetInt(interp, (char*)args[1].c_str(), &first)!=TCL_OK)
            return TCL_ERROR;
    }
    if (numargs>1) {
        if (Tcl_GetInt(interp, (char*)args[1].c_str(), &n)!=TCL_OK)
            return TCL_ERROR;
    }
    last=first+n;
    if (last>=num)
        last=num-1;
        
    for (int i=first; i<=last; i++) {
        snprintf(s, 32, format, ib.getint());
        ss << s;
        if (i<last) ss << ' ';
    }
    return TCL_OK;
}
/*****************************************************************************/

int E_ved::f_stringlist(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
C_inbuf ib(conf->buffer(), conf->size());
ib++; // errorcode ueberspringen
int num=ib.getint();
for (int i=0; i<num; i++)
  {
  STRING st;
  ib >> st;
  ss << '{' << st << '}';
  if (i+1<num) ss << ' ';
  }
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_string(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
C_inbuf ib(conf->buffer(), conf->size());
ib++; // errorcode ueberspringen
STRING st;
ib >> st;
ss << st;
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_identify(OSTRINGSTREAM& ss, const C_confirmation* conf, int needtest,
    int numargs, const STRING* args)
{
C_inbuf ib(conf->buffer(), conf->size());
ib++; // errorcode ueberspringen
for (int i=0; i<3; i++) // ver_ved, ver_req, ver_unsol
    ss << ib.getint() << " ";
int num=ib.getint();
for (int j=0; j<num; j++)
  {
  STRING s;
  ib >> s;
  ss << '{' << s << '}';
  if (j+1<num) ss << ' ';
  }
return TCL_OK;
}

/*****************************************************************************/
int E_ved::f_domevent(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
    int num=-1;
    if (numargs>0) {
        if (Tcl_GetInt(interp, (char*)args[0].c_str(), &num)!=TCL_OK)
            return TCL_ERROR;
    }

    C_inbuf ib(conf->buffer(), conf->size());
    ib++; // errorcode ueberspringen
    unsigned int evnum, trigger, numis;
    //cerr << ib << endl;
    ib >> evnum >> trigger >> numis;
    ss << evnum << ' ';
    ss << hex << setfill('0') << "0x" << setw(4) << trigger << dec << setfill(' ');
    ss << ' ' << numis << " {";
    for (unsigned int is=0; is<numis; is++) {
        unsigned int isid, issize;
        ib >> isid >> issize;
        if (is>0) ss<<' ';
        ss << '{' << isid << ' ' << issize << " {";
        ss << hex << setfill('0');
        for (int i=0; i<(int)issize; i++) {
            if ((num<0) || (i<num)) {
                if (i>0) ss << ' ';
                ss << "0x" << setw(8) << (unsigned int)ib.getint();
            } else {
                ib++;
            }
        }
        ss << "}}" << dec << setfill(' ');
    }
    ss << '}';

    return TCL_OK;
}
/*****************************************************************************/

int E_ved::f_readoutstatus(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
C_inbuf ib(conf->buffer(), conf->size());
C_readoutstatus* rostatus=new C_readoutstatus(ib, 1);

switch (rostatus->status())
  {
  case -3: ss << "error"; break;
  case -2: ss << "no_more_data"; break;
  case -1: ss << "stopped"; break;
  case 0: ss << "inactive"; break;
  case 1: ss << "running"; break;
  default: ss << rostatus->status(); break;
  }
ss<<' '<<rostatus->eventcount();
if (rostatus->time_valid())
  {
  ss<<' '<<setprecision(20)<<rostatus->gettime();
  for (int a=0; a<rostatus->numaux(); a++)
    {
    ss<<" {"<<rostatus->auxkey(a)<<" {";
    for (int aa=0; aa<rostatus->auxsize(a); aa++)
      {
      ss<<rostatus->aux(a, aa);
      if (aa+1<rostatus->auxsize(a)) ss<<' ';
      }
    ss<<"}}";
    }
  }
delete rostatus;
// ss << ' ' << ib.getint(); // eventcount
// if (ib.size()>3)
//   {
//   double sec;
//   int aux=ib.getint();
//   switch (aux)
//     {
//     case 1:
//       sec=ib.getint()+(double)ib.getint()/1000000.0;
//       break;
//     case 2:
//       sec=86400*(ib.getint()-2440587)+ib.getint()+double(ib.getint())/100.0;
//       break;
//     default:
//       sec=0;
//       break;
//     }
//   ss << ' ' << setprecision(20) << sec;
//   }
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_ioaddr(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
if (numargs!=1)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args for f_ioaddr; must be in|out", -1));
  return TCL_ERROR;
  }
io_direction io;
if (args[0].compare("in")==0)
  io=io_in;
else if (args[0].compare("out")==0)
  io=io_out;
else
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong args for f_ioaddr; must be in|out", -1));
  return TCL_ERROR;
  }
C_data_io* addr=C_data_io::create(io, conf);
int res=ff_ioaddr(ss, addr);
delete addr;
return res;
}

/*****************************************************************************/

int E_ved::ff_ioaddr(OSTRINGSTREAM& ss, const C_data_io* addr)
{
// output:
//   iotyp buffersize prior addresstype {address}
ss << (*addr);
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_dostatus(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
if (numargs!=0)
  {
  cerr<<"E_ved::f_dostatus: numargs must be 0"<<endl;
  }
C_dataoutstatus status(conf);
ss << status;
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_modullist(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
int version;
if (numargs>0)
  {
  if (Tcl_GetInt(interp, (char*)args[0].c_str(), &version)!=TCL_OK)
      return TCL_ERROR;
  }
else
  version=0;

C_modullist* mlist=new C_modullist(conf, version);
ff_modullist(ss, mlist, version);
delete mlist;
return TCL_OK;
}

/*****************************************************************************/
void
E_ved::ff_modullist(OSTRINGSTREAM& ss, const C_modullist* mlist, int version)
{
    int n=mlist->size();
    if (version) {
        ss << hex << setiosflags(ios::showbase);
        for (int i=0; i<n; i++) {
            const C_modullist::ml_entry& entry=mlist->get(i);
            if (i>0)
                ss << ' ';
            switch (entry.modulclass) {
            case modul_none:
                ss<<'{'<<"none"<<'}';
                break;
            case modul_unspec:
                ss<<'{'<<"unspec"<<' '<<entry.modultype<<' '
                    <<entry.address.unspec.addr<<'}';
                break;
            case modul_generic:
                break;
            case modul_fastbus:
            case modul_camac:
            case modul_vme:
            case modul_lvd:
            case modul_can:
            case modul_perf:
            case modul_caenet:
            case modul_sync:
            case modul_pcihl:
                ss<<'{'<<Modulclass_names[entry.modulclass]<<' '
                    <<entry.modultype<<' '<<entry.address.adr2.addr<<' '
                    <<entry.address.adr2.crate<<'}';
                break;
            case modul_invalid:
                ss<<'{'<<"invalid_"<<(int)entry.modulclass<<'}';
            }
        }
    } else {
        ss << hex << setiosflags(ios::showbase);
        for (int i=0; i<n; i++) {
            const C_modullist::ml_entry& entry=mlist->get(i);
            if (i>0)
                ss << ' ';
            ss<<'{'<<entry.address.unspec.addr<<' '<<entry.modultype<<'}';
        }
    }
}
/*****************************************************************************/

int E_ved::f_lamstatus(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
    int i, num;

    C_inbuf ib(conf->buffer(), conf->size());
    //ss<<ib<<endl;
    ib++; // errorcode ueberspringen
    for (i=0; i<4; i++)
        ss << ib.getint() << ' ';
    ss << procname(ib.getint(), Capab_trigproc);
    num=ib.getint();
    ss << " {";
    for (i=0; i<num; i++) {
        ss << ib.getint();
        if (i+1<num)
            ss << ' ';
    }
    ss << '}';
    return TCL_OK;
}
/*****************************************************************************/

int E_ved::f_isstatus(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
C_isstatus* status=new C_isstatus(conf);
ff_isstatus(ss, status);
delete status;
return TCL_OK;
}

/*****************************************************************************/

void E_ved::ff_isstatus(OSTRINGSTREAM& ss, const C_isstatus* status)
{
ss << status->id() << ' ' << status->enabled() << ' ' << status->members()
    << " {";
for (int i=0, n=status->num_trigger(); i<n; i++)
  {
  if (i>0) ss << ' ';
  ss << status->trigger(i);
  }
ss << '}';
}

/*****************************************************************************/

int E_ved::f_readoutlist(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
// (1): priority (2): number of procedures
//ss << conf->buffer(1) << ' ' << conf->buffer(2) << " {";
ss << conf->buffer(1) << " {";
int i=3;
while (i<conf->size())
  {
  if (i>3) ss << ' ';
  ss << procname(conf->buffer(i++), Capab_listproc) << " {";
  int n=conf->buffer(i++);
  for (int j=0; j<n; j++)
    {
    if (j>0) ss << ' ';
    ss << conf->buffer(i++);
    }
  ss << '}';
  }
ss << '}';
return TCL_OK;
}

/*****************************************************************************/

int E_ved::f_createis(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
if (numargs!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args for f_createis; must be open|create idx",
      -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, (char*)(args[1].c_str()), &idx)!=TCL_OK)
    return TCL_ERROR;
if (args[0]=="open")
  {
  C_inbuf ib(conf->buffer(), conf->size());
  ib++;
  int num=ib.getint();
  int found=0;
  for (int i=0; !found && (i<num); i++) if ((int)ib.getint()==idx) found=1;
  if (!found)
    {
    OSTRINGSTREAM ss;
    ss << "is " << idx << "does not exist";
    Tcl_SetResult_Stream(interp, ss);
    return TCL_ERROR;
    }
  }
else if (args[0]=="create")
  {
  // nothing to do
  }
else
  {
  OSTRINGSTREAM ss;
  ss << "unknown arg " << '"' << args[0] << '"'
      << "; possible args are open and create";
  Tcl_SetResult_Stream(interp, ss);
  return TCL_ERROR;
  }
E_is* is;
is=new E_is(interp, this, idx, C_instr_system::notest);
is->create_tcl_proc();
ss << is->origtclprocname();
return TCL_OK;
}

/*****************************************************************************/
int
E_ved::f_lamupload(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
    C_inbuf ib(conf->buffer(), conf->size());
    ib++; // skip errorcode
    ss<<ib.getint()<<" {"; // output enable flag
    int m=ib.getint();     // number of procedures
    for (int i=0; i<m; i++) {
        if (i>0)
            ss<<' ';
        ss<<procname(ib.getint(), Capab_listproc)<<" {";
        int n=ib.getint(); // number of arguments of current procedure
        for (int j=0; j<n; j++) {
            if (j>0)
                ss<<' ';
            ss<<ib.getint();
        }
        ss<<'}';
    }
    ss <<'}';

    return TCL_OK;
}
/*****************************************************************************/

int E_ved::f_trigger(OSTRINGSTREAM& ss, const C_confirmation* conf,
    int needtest, int numargs, const STRING* args)
{
C_inbuf ib(conf->buffer(), conf->size());
ib++; // errorcode ueberspringen
ss << procname(ib.getint(), Capab_trigproc);
int n=ib.getint();
for (int i=0; i<n; i++) ss << ' ' << ib.getint();
return TCL_OK;
}

/*****************************************************************************/
/*****************************************************************************/
