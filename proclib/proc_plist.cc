/*
 * proc_plist.cc
 * 
 * created: 18.08.94 PW
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 * 24.02.1999 PW: add_par(float) and add_par(double) added
 */

#include <cerrno>
#include <stdarg.h>
#include <cstdlib>
#include <cstring>
#include <ved_errors.hxx>
#include "proc_plist.hxx"
#include <proc_ved.hxx>
#include <caplib.hxx>
#include <clientcomm.h>
#include <xdrstring.h>
#include <versions.hxx>
#include "proc_is.hxx"

VERSION("2008-Nov-15", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_plist.cc,v 2.18 2009/02/05 19:42:05 wuestner Exp $")
#define XVERSION

#undef PROCDEBUG
#ifdef PROCDEBUG
# define PD(x) x
#else
# define PD(x)
#endif

/*****************************************************************************/

C_proclist::C_proclist(const char* text, C_VED* ved, C_instr_system* is,
    T_smartptr<C_capability_list>& caplist)
:ved(ved), is(is), caplist(caplist), listsize(100), list_idx(1), proc_num_(0)
{
// {
// OSTRINGSTREAM ss;
// ss<<"VED "<<ved->name();
// if (is)
//   ss<<" IS "<<is->is_idx();
// else
//   ss<<" IS0";
// ss<<" called from "<<text<<ends;
// char* s=ss.str();
// cerr<<"C_proclist["<<s<<"]::C_proclist(); this="<<(void*)this<<endl;
// delete[] s;
// }
list=new ems_u32[listsize];
if (list==0) throw new C_ved_error(ved, errno);
}

/*****************************************************************************/

C_proclist::~C_proclist()
{
PD(cerr<<"C_proclist["<<name()<<"]::~C_proclist(); this="<<(void*)this<<endl;)
delete[] list;
}

/*****************************************************************************/
STRING C_proclist::name() const
{
OSTRINGSTREAM ss;
ss<<"VED "<<ved->name();
if (is)
  ss<<" IS >"<<is->name()<<"<";
else
  ss<<" IS0";
ss<<" ("<<(void*)this<<")"<<ends;
STRING st=ss.str();
return st;
}
/*****************************************************************************/

void C_proclist::clear()
{
PD(cerr<<"C_proclist["<<name()<<"]::clear()"<<endl;)
list_idx=1;
proc_num_=0;
}

/*****************************************************************************/

void C_proclist::growlist(int num)
{
if (listsize>=(list_idx+num))
  return;
else
  {
  ems_u32 *help;
  int i;
  listsize=list_idx+num+100;
//  help=(int*)malloc(listsize*sizeof(int));
  help=new ems_u32[listsize];
  if (help==0) throw new C_ved_error(ved, errno);
  for (i=0; i<list_idx; i++) help[i]=list[i];
  delete[] list;
  list=help;
  }
}

/*****************************************************************************/

void C_proclist::listend()
{
PD(cerr<<"C_proclist["<<name()<<"]::listend()"<<endl;)
list[0]=proc_num_;
if (proc_num_) list[proc_idx+1]=list_idx-proc_idx-2;
}

/*****************************************************************************/

void C_proclist::printlist(ostream& ss)
{
int i;
unsigned int j;
ems_u32* ptr;

listend();
if (proc_num_==0)
  ss << "list is empty" << ends;
else
  {
  for (i=0; i<list_idx; i++) cout << list[i] << ", ";
  cout << endl;
  ptr=list+1;
  for (i=0; i<proc_num_; i++)
    {
    ss << caplist->get(*ptr) << "(";
    for (j=0; j<ptr[1]; j++) ss << ptr[j+2] << ((j+1<ptr[1])?", ":"");
    ss << ")" << endl;
    ptr+=ptr[1]+2;
    }
  ss << ends;
  }
}

/*****************************************************************************/

int C_proclist::add_proc(int proc)
{
PD(cerr<<"C_proclist["<<name()<<"]::add_proc(proc="<<proc<<")"<<endl;)
growlist(2);
if (proc_num_>0)
  {
  list[proc_idx+1]=list_idx-proc_idx-2;
  }
list[list_idx]=proc;
proc_idx=list_idx;
proc_num_++;
list_idx+=2;
return(0);
}

/*****************************************************************************/

int C_proclist::add_proc(const char* proc)
{
PD(cerr<<"C_proclist["<<name()<<"]::add_proc(proc="<<proc<<")"<<endl;)
int iproc;

if ((iproc=caplist->get(proc))==-1)
  return(-1);
else
  return(add_proc(iproc));
}

/*****************************************************************************/
int C_proclist::add_par(int par)
{
    PD(cerr<<"C_proclist["<<name()<<"]::add_par(int par="<<par<<")"<<endl;)
    if (proc_num_==0)
            throw new C_ved_error(ved, "C_proclist: parameter without procedure");
    growlist(1);
    list[list_idx++]=par;
    return 0;
}
/*****************************************************************************/
int C_proclist::mod_par(int pos, int par)
{
    PD(cerr<<"C_proclist["<<name()<<"]::add_par(int par="<<par<<")"<<endl;)
    if (proc_num_==0)
            throw new C_ved_error(ved, "C_proclist: parameter without procedure");
    if ((list_idx-1<pos) || (list_idx<1)) {
        throw new C_ved_error(ved, "C_proclist: mod_par: invalid position");
    }
    list[pos]=par;
    return 0;
}
/*****************************************************************************/
int C_proclist::add_par(ems_u32 par)
{
    PD(cerr<<"C_proclist["<<name()<<"]::add_par(ems_u32 par="<<par<<")"<<endl;)
    if (proc_num_==0)
            throw new C_ved_error(ved, "C_proclist: parameter without procedure");
    growlist(1);
    list[list_idx++]=par;
    return 0;
}
/*****************************************************************************/
int C_proclist::mod_par(int pos, ems_u32 par)
{
    PD(cerr<<"C_proclist["<<name()<<"]::add_par(ems_u32 par="<<par<<")"<<endl;)
    if (proc_num_==0)
            throw new C_ved_error(ved, "C_proclist: parameter without procedure");
    if ((list_idx-1<pos) || (list_idx<1)) {
        throw new C_ved_error(ved, "C_proclist: mod_par: invalid position");
    }
    list[pos]=par;
    return 0;
}
/*****************************************************************************/
#if 0
int C_proclist::add_par(float par)
{
    PD(cerr<<"C_proclist["<<name()<<"]::add_par(float par="<<par<<")"<<endl;)
    if (proc_num_==0)
        throw new C_ved_error(ved, "C_proclist: parameter without procedure");
    growlist(1);
    union {
        ems_u32 i;
        float f;
    } u;
    u.f=par;
    list[list_idx++]=u.i;
    return 0;
}
#endif
/*****************************************************************************/
#if 0
int C_proclist::add_par(double par)
{
    PD(cerr<<"C_proclist["<<name()<<"]::add_par(double par="<<par<<")"<<endl;)
    if (proc_num_==0)
        throw new C_ved_error(ved, "C_proclist: parameter without procedure");
    growlist(2);
    union {
        ems_u32 i[2];
        double d;
    } u;
    u.d=par;

    list[list_idx++]=u.i[1];
    list[list_idx++]=u.i[0];
    return 0;
}
#endif
/*****************************************************************************/

int C_proclist::add_par(int num, int par, ...)
{
PD(cerr<<"C_proclist["<<name()<<"]::add_par(par=...)"<<endl;)
va_list vl;
int i;

if (proc_num_==0)
    throw new C_ved_error(ved, "C_proclist: parameters without procedure");
growlist(num);
va_start(vl, par);
list[list_idx++]=par;
num--;
for (i=0; i<num; i++)
  {
  list[list_idx++]=va_arg(vl, int);
  }
va_end(vl);
return(0);
}

/*****************************************************************************/
int C_proclist::add_par(int num, ems_u32* par)
{
    PD(cerr<<"C_proclist["<<name()<<"]::add_par(*)"<<endl;)

    int i;

    if (proc_num_==0)
        throw new C_ved_error(ved, "C_proclist: parameters without procedure");

    growlist(num);
    for (i=0; i<num; i++) {
        list[list_idx++]=par[i];
    }

    return(0);
}
/*****************************************************************************/

int C_proclist::add_par(const char* par)
{
PD(cerr<<"C_proclist["<<name()<<"]::add_par(par="<<par<<")"<<endl;)
int len;

if (proc_num_==0)
    throw new C_ved_error(ved, "C_proclist: parameters without procedure");

len=strxdrlen(par);
growlist(len);

outstring(list+list_idx, par);
list_idx+=len;

return(0);
}

/*****************************************************************************/

int C_proclist::send_request(int is)
{
PD(cerr<<"C_proclist["<<name()<<"]::send_request(is="<<is<<")"<<endl;)
int xid;

if (proc_num_==0)
    throw new C_ved_error(ved, "C_proclist: Procedurelist is empty");
listend();
PD(printlist(cerr);)
if ((xid=DoCommand(ved->ved, is, list_idx, list))==-1)
  {
  OSTRINGSTREAM s;
  s << "Can't send command" << ends;
  throw new C_ved_error(ved, EMS_errno, s);
  }
// clear();
return(xid);
}

/*****************************************************************************/

void C_proclist::getlist(ems_u32** liste, int* size)
{
PD(cerr<<"C_proclist["<<name()<<"]::getlist(...)"<<endl;)

// if (proc_num_==0)
//   {
//   *liste=0;
//   *size=0;
//   }
// else
//   {
  listend();
  *liste=list;
  *size=list_idx;
//   }
// cerr << "C_proclist::getlist: liste="<<(void*)*liste<<"; size="<<*size<<endl;
}

/*****************************************************************************/
/*****************************************************************************/
