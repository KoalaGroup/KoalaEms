/*
 * emstcl_confir.cc
 * created 06.09.95
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "emstcl_ved.hxx"
#include <cerrno>
#include <errors.hxx>
#include "findstring.hxx"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl_confir.cc,v 1.13 2014/07/14 15:13:25 wuestner Exp $")
#define XVERSION

const E_confirmation** E_confirmation::econfirmations=0;
int E_confirmation::num_econfirmations=0;
int E_confirmation::max_econfirmations=0;

using namespace std;

/*****************************************************************************/

E_confirmation::E_confirmation(Tcl_Interp* interp, E_ved* ved,
    const C_confirmation& conf)
:deleted(0), interp(interp), ved(ved) 
{
ostringstream st;
st << "conf_" << conf.header()->ved << "_" << conf.header()->transid;
string rootname=st.str();
int idx=0;
Tcl_CmdInfo info;
origprocname=rootname;
while (Tcl_GetCommandInfo(interp, (char*)origprocname.c_str(), &info)!=0)
  {
  idx++;
  ostringstream ss;
  ss << rootname << '#' << idx;
  origprocname=ss.str();
  }
tclcommand=Tcl_CreateCommand(interp, (char*)origprocname.c_str(),
        E_confirmation_Ems_ConfirmationCommand,
        ClientData(this), E_confirmation_Ems_ConfirmationDelete);
confirmation=new C_confirmation(conf);
reg();
}

/*****************************************************************************/

E_confirmation::~E_confirmation()
{
unreg();
delete confirmation;
if (!deleted) Tcl_DeleteCommand(interp, Tcl_GetCommandName(interp, tclcommand));
}

/*****************************************************************************/

void E_confirmation_Ems_ConfirmationDelete(ClientData clientdata)
{
((E_confirmation*)clientdata)->deleted=1;
delete (E_confirmation*)clientdata;
}

/*****************************************************************************/

string E_confirmation::tclprocname() const
{
string s;
s=Tcl_GetCommandName(interp, tclcommand);
return s;
}

/*****************************************************************************/

void E_confirmation::reg(void) const
{
if (num_econfirmations==max_econfirmations)
  {
  const E_confirmation** help=new const E_confirmation*[num_econfirmations+10];
  for (int i=0; i<num_econfirmations; i++) help[i]=econfirmations[i];
  delete[] econfirmations;
  econfirmations=help;
  max_econfirmations=num_econfirmations+10;
  }
econfirmations[num_econfirmations++]=this;
}

/*****************************************************************************/

void E_confirmation::unreg(void) const
{
int i=0;
while ((i<num_econfirmations) && (econfirmations[i]!=this)) i++;
if (i==num_econfirmations)
  {
  cerr << "E_confirmation::unreg("<<(void*)this<<"): not found" << endl;
  return;
  }
for (int j=i; j<(num_econfirmations-1); j++)
    econfirmations[j]=econfirmations[j+1];
num_econfirmations--;
}

/*****************************************************************************/

int E_confirmation_Ems_ConfirmationCommand(ClientData clientdata,
    Tcl_Interp* interp, int argc, const char* argv[])
{
int res=((E_confirmation*)clientdata)->ConfirmationCommand(argc, argv);
return res;
}

/*****************************************************************************/

int E_confirmation::ConfirmationCommand(int argc, const char* argv[])
{
// <Conf> print|delete|get
int res;
if (argc<2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("argument expected", -1));
  return TCL_ERROR;
  }
const char* names[]={
  "print",
  "delete",
  "convert",
  "get",
  0};
switch (findstring(interp, argv[1], names))
  {
  case 0: res=e_print(argc, argv); break;
  case 1: res=e_delete(argc, argv); break;
  //case 2: res=e_convert(argc, argv); break;
  case 3: res=e_get(argc, argv); break;
  default: res=TCL_ERROR; break;
  }
return res;
}

/*****************************************************************************/

string E_confirmation::new_E_confirmation(Tcl_Interp* interp, E_ved* ved,
    const C_confirmation& conf)
{
E_confirmation* econf=new E_confirmation(interp, ved, conf);
return econf->tclprocname();
}

/*****************************************************************************/
int E_confirmation::e_delete(int argc, CONST char* argv[])
{
// <Conf> delete
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be delete", -1));
  return TCL_ERROR;
  }
Tcl_DeleteCommand(interp, argv[0]);
return TCL_OK;
}
/*****************************************************************************/

int E_confirmation::e_print(int argc, const char* argv[])
{
// <Conf> print raw|text|tabular
E_ved::confbackbox confback;
int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be raw|text|tabular", -1));
  return TCL_ERROR;
  }
const char* names[]={
  "raw",
  "text",
  "tabular",
  0};
switch (findstring(interp, argv[2], names))
  {
  case 0:
    res=confback.finit(interp, ved, argc, argv, &E_ved::f_printconf_raw);
    break;
  case 1:
    res=confback.finit(interp, ved, argc, argv, &E_ved::f_printconf_text);
    break;
  case 2: 
    res=confback.finit(interp, ved, argc, argv, &E_ved::f_printconf_tabular);
    break;
  default: res=TCL_ERROR; break;
  }
if (res!=TCL_OK) return TCL_ERROR;
ostringstream ss;
if (confback.callformer(ved, ss, confirmation)!=TCL_OK) return TCL_ERROR;
string s=ss.str();
Tcl_SetObjResult(interp, Tcl_NewStringObj(s.c_str(), -1));
return TCL_OK;
}
/*****************************************************************************/
int E_confirmation::e_get(int argc, const char* argv[])
{
// <Conf> get size|client|ved|reqtype|flags|xid|body
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be get size|client|ved|reqtype|flags|xid|(body idx)", -1));
  return TCL_ERROR;
  }
const char* names[]={
  "size",
  "client",
  "ved",
  "reqtype",
  "flags",
  "xid",
  "body",
  0};
ostringstream ss;
int res=TCL_OK;
switch (findstring(interp, argv[2], names))
  {
  case 0:
    ss<<confirmation->header()->size;
    break;
  case 1:
    ss<<confirmation->header()->client;
    break;
  case 2:
    ss<<confirmation->header()->ved;
    break;
  case 3:
    ss<<confirmation->header()->type.reqtype;
    break;
  case 4:
    ss<<confirmation->header()->flags;
    break;
  case 5:
    ss<<confirmation->header()->transid;
    break;
  case 6:
    if (argc<4)
      {
      int max=confirmation->header()->size;
      for (int i=0; i<max; i++)
        {
        ss<<confirmation->buffer(i);
        if (i<max+1) ss<<" ";
        }
      }
    else
      {
      int idx;
      if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) {
        cerr<<"E_confirmation::e_get: Tcl_GetInt("<<argv[3]<<") failed"<<endl;
        return TCL_ERROR;
      }
      ss<<confirmation->buffer(idx);
      }
    break;
  default: res=TCL_ERROR; break;
  }
if (res==TCL_OK)
  {
  string s=ss.str();
  //cerr<<"E_confirmation::e_get: result is "<<s<<endl;
  Tcl_SetObjResult(interp, Tcl_NewStringObj(s.c_str(), -1));
  }
return TCL_OK;
}
/*****************************************************************************/
/*****************************************************************************/
