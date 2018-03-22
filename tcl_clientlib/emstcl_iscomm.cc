/*
 * emstcl_iscomm.cc
 * 
 * created 07.09.95 PW
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "emstcl.hxx"
#include "emstcl_is.hxx"
#include <ved_errors.hxx>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include "findstring.hxx"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl_iscomm.cc,v 1.27 2015/04/24 16:16:45 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

int E_is::e_close(int argc, const char* argv[])
//close
{
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be close", -1));
  return TCL_ERROR;
  }
Tcl_DeleteCommand(interp, argv[0]);
return TCL_OK;
}

/*****************************************************************************/

int E_is::e_delete(int argc, const char* argv[])
// delete
{
int res;
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be delete", -1));
  return TCL_ERROR;
  }
int idx=is_idx();
E_ved* ved=eved;
try
  {
  ved->DeleteIS(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    ved->installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  res=TCL_ERROR;
  }
res=TCL_OK;
destroy=1;
Tcl_DeleteCommand(interp, argv[0]);
return res;
}

/*****************************************************************************/
int E_is::e_closecommand(int argc, const char* argv[])
//closecommand
// wird kurz vor $is close und rename $is {} ausgefuehrt
// %n wird durch realen VEDnamen ersetzt
// %v wird durch vedcommando ersetzt
// %i wird durch index ersetzt
// %c wird durch Kommandonamen ersetzt
{
    if (argc>3) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be closecommand [command]",
            -1));
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(ccomm.c_str(), -1));
    ccomm=argc==3?argv[2]:"";
    return TCL_OK;
}
/*****************************************************************************/
int E_is::e_deletecommand(int argc, const char* argv[])
//deletecommand
// wird kurz vor $is delete ausgefuehrt
// %n wird durch realen VEDnamen ersetzt
// %v wird durch vedcommando ersetzt
// %i wird durch index ersetzt
// %c wird durch Kommandonamen ersetzt
{
    if (argc>3) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be deletecommand [command]",
            -1));
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(dcomm.c_str(), -1));
    dcomm=argc==3?argv[2]:"";
    return TCL_OK;
}
/*****************************************************************************/

int E_is::e_id(int argc, const char* argv[])
{
// <IS> id
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_integer)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be id", -1));
  return TCL_ERROR;
  }
try
  {
  if (confmode_==synchron)
    {
    int id=is_id();
    ostringstream ss;
    ss << id;
    Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    is_id();
    confback.xid(last_xid_);
    (eved)->installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_is::e_idx(int argc, const char* argv[])
//idx
{
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be idx", -1));
  return TCL_ERROR;
  }
ostringstream ss;
ss << is_idx();
Tcl_SetResult_Stream(interp, ss);
return TCL_OK;
}

/*****************************************************************************/
int E_is::e_ved(int argc, const char* argv[])
//ved
{
    if (argc!=2) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be ved", -1));
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp,
        Tcl_NewStringObj(eved->tclprocname().c_str(), -1));
    return TCL_OK;
}
/*****************************************************************************/

int E_is::e_caplist(int argc, const char* argv[])
{
// <VED> caplist
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be caplist", -1));
  return TCL_ERROR;
  }
try
  {
  string name;
  int num=numprocs();
  for(int id=0; id<num; id++)
    {
    ostringstream ss;
    ss << id << ' ' << procname(id);
    Tcl_AppendElement_Stream(interp, ss);
    }
  }
catch(C_error* error)
  {
  Tcl_SetResult_Err(interp, error);
  delete error;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/
int
E_is::e_command(int argc, const char* argv[])
{
// <is> command {name1 {arg1 arg2 ...} name2 {arg1 args ...} ...}
//
{
    const char* var=Tcl_GetVar(interp, "ems_debug", TCL_GLOBAL_ONLY);
    int ivar;

    if (var && (Tcl_GetInt(interp, var, &ivar)==TCL_OK) && (ivar>0)) {
        cerr<<"command for IS "<<argv[0]<<": "<<argc<<" arguments"<<endl;
        for (int i=0; i<argc; i++)
            cerr<<"["<<i<<"]: >"<<argv[i]<<"<"<<endl;
        }
    }

    E_ved::confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, eved, argc, argv, &E_ved::f_command)!=TCL_OK)
            return TCL_ERROR;
    } else {
        if (confback.finit(interp, eved, argc, argv, &E_ved::f_command)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc!=3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be\n"
            "  <is> command {name1 {arg1 arg2 ...} name2 {arg1 args ...} ...}",
            -1));
        return TCL_ERROR;
    }

    int plc;
    const char** plv;
    if (Tcl_SplitList(interp, argv[2], &plc, &plv)!=TCL_OK)
        return TCL_ERROR;
    if (plc%2) {
        Tcl_Free((char*)plv);
        Tcl_SetObjResult(interp, Tcl_NewStringObj("arguments missing", -1));
        return TCL_ERROR;
    }

    int res=TCL_OK;
    exec_mode oldmode=execution_mode(delayed);
    clear_command();

    for (int n=0; (res==TCL_OK) && (n<plc); n+=2) {
        int pc;
        const char** pv;
        if (Tcl_SplitList(interp, plv[n+1], &pc, &pv)==TCL_OK) {
            try {
                command(plv[n]);
                for (int i=0; (res==TCL_OK) && (i<pc); i++) {
                    if (pv[i][0]=='\'') {
                        char* s=(char*)pv[i];
                        char* se=s+strlen(s)-1;
                        if (*se!='\'') {
                            ostringstream ss;
                            ss << "Unmatched ' in " << s;
                            Tcl_SetResult_Stream(interp, ss);
                            res=TCL_ERROR;
                        } else {
                            *se=0;
                            add_param(s+1);
                            *se='\'';
                        }
                    } else {
                        long val;
                        if (xTclGetLong(interp, (char*)pv[i], &val)!=TCL_OK)
                            res=TCL_ERROR;
                        else
                            add_param((int)val);
                    }
                }
            } catch(C_error* e) {
                Tcl_SetResult_Err(interp, e);
                delete e;
                res=TCL_ERROR;
            }
            Tcl_Free((char*)pv);
        } else {
            res=TCL_ERROR;
        }
    }
    Tcl_Free((char*)plv);

    if (res==TCL_ERROR) {
        clear_command();
        execution_mode(oldmode);
        return res;
    }

    try {
        if (confmode_==synchron) {
            C_confirmation* conf=execute();
            ostringstream ss;
            //(eved)->f_command(ss, conf, 0, 0, 0);
            confback.callformer(eved, ss, conf);
            Tcl_SetResult_Stream(interp, ss);
            delete conf;
        } else {
            execute();
            confback.xid(last_xid_);
            (eved)->installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        res=TCL_ERROR;
    }
    execution_mode(oldmode);
    return res;
}

/*****************************************************************************/

int E_is::e_command1(int argc, const char* argv[])
{
// <is> command1 name [arg1 ...]
//
{
const char* var=Tcl_GetVar(interp, "ems_debug", TCL_GLOBAL_ONLY);
int ivar;
if (var && (Tcl_GetInt(interp, var, &ivar)==TCL_OK) && (ivar>0))
  {
  cerr<<"command1 for IS "<<argv[0]<<": "<<argc<<" arguments"<<endl;
  for (int i=0; i<argc; i++)
    cerr<<"["<<i<<"]: >"<<argv[i]<<"<"<<endl;
  }
}
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_command)!=TCL_OK)
      return TCL_ERROR;
  }
else
  {
  if (confback.finit(interp, eved, argc, argv, &E_ved::f_command)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be\n"
  "  <is> command1 name [arg1 ...]", -1));
  return TCL_ERROR;
  }
int res=TCL_OK;
exec_mode oldmode=execution_mode(delayed);
clear_command();
try
  {
  command(argv[2]);
  for (int i=3; (res==TCL_OK) && (i<argc); i++)
    {
    int length=strlen(argv[i]);
    const char* p=index(argv[i], '.');
    if (argv[i][0]=='\'')
      {
      char* s=(char*)argv[i];
      char* se=s+length-1;
      if (*se!='\'')
        {
        ostringstream ss;
        ss << "Unmatched ' in " << s;
        Tcl_SetResult_Stream(interp, ss);
        res=TCL_ERROR;
        }
      else
        {
        *se=0;
        add_param(s+1);
        *se='\'';
        }
      }
    //else if (p || (argv[i][length-1]=='d') || (argv[i][length-1]=='f'))
    else if (p)
      {
#if 1
        Tcl_SetObjResult(interp, Tcl_NewStringObj("float argument not supported", -1));
        res=TCL_ERROR;
#else
      double val;
      if (Tcl_GetDouble(interp, argv[i], &val)!=TCL_OK)
        res=TCL_ERROR;
      else
        if (argv[i][length-1]=='f')
          add_param(val);
        else
          add_param((float)val);
#endif
      }
    else
      {
      long val;
      if (xTclGetLong(interp, (char*)argv[i], &val)!=TCL_OK)
        res=TCL_ERROR;
      else
        add_param((int)val);
      }
    }
  if (res==TCL_OK)
    {
    if (confmode_==synchron)
      {
      C_confirmation* conf=execute();
      ostringstream ss;
      //(eved)->f_command(ss, conf, 0, 0, 0);
      confback.callformer(eved, ss, conf);
      Tcl_SetResult_Stream(interp, ss);
      delete conf;
      }
    else
      {
      execute();
      confback.xid(last_xid_);
      (eved)->installconfback(confback);
      ostringstream ss;
      ss << last_xid_;
      Tcl_SetResult_Stream(interp, ss);
      }
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  res=TCL_ERROR;
  }
execution_mode(oldmode);
return res;
}

/*****************************************************************************/

int E_is::e_memberlist(int argc, const char* argv[])
{
// <IS> memberlist create|delete|upload
//
int res;
if (argc==2)
  res=e_memlist_upload(argc, argv);
else
  {
  static const char* names[]={
    "create",
    "delete",
    "upload",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_memlist_create(argc, argv); break;
    case 1: res=e_memlist_delete(argc, argv); break;
    case 2: res=e_memlist_upload(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/
int E_is::e_memlist_create(int argc, const char* argv[])
{
// <is> memberlist create {addr1 addr2 ...}
// <is> memberlist create addr1 addr2 ...
//
    E_ved::confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, eved, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    int mlc;
    ems_u32* mlist;
    switch (argc) {
    case 3: // memberlist ist leer
        mlc=0;
        mlist=0;
        break;
    case 4: { // memberlist besteht aus einer Liste
        const char** mlv;
        if (Tcl_SplitList(interp, argv[3], &mlc, &mlv)!=TCL_OK)
            return TCL_ERROR;
        mlist=new ems_u32[mlc];
        for (int i=0; i<mlc; i++) {
            if (Tcl_GetInt_u32(interp, mlv[i], mlist+i)!=TCL_OK) {
                Tcl_Free((char*)mlv);
                delete[] mlist;
                return TCL_ERROR;
            }
        }
        Tcl_Free((char*)mlv);
    }
    break;
    default: { // memberlist besteht aus Einzelwerten
        mlc=argc-3;
        mlist=new ems_u32[mlc];
        for (int i=0; i<mlc; i++) {
            if (Tcl_GetInt_u32(interp, argv[i+3], mlist+i)!=TCL_OK) {
                delete[] mlist;
                return TCL_ERROR;
            }
        }
    }
    break;
    }

    int res;
    try {
        DownloadISModullist(mlc, mlist);
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            (eved)->installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
        res=TCL_OK;
    } catch(C_error* e) {
        res=TCL_ERROR;
        Tcl_SetResult_Err(interp, e);
        delete e;
    }
    delete[] mlist;
    return res;
}

/*****************************************************************************/

int E_is::e_memlist_upload(int argc, const char* argv[])
{
// <is> memberlist [upload]
//
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_integerlist)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc>3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be memberlist [upload]",
      -1));
  return TCL_ERROR;
  }
try
  {
  if (confmode_==synchron)
    {
    C_memberlist* mlist=UploadISModullist();
    int n=mlist->size();
    for (int i=0; i<n; i++)
      {
      ostringstream ss;
      ss << hex << setiosflags(ios::showbase) << (*mlist)[i];
      Tcl_AppendElement_Stream(interp, ss);
      }
    }
  else
    {
    UploadISModullist();
    confback.xid(last_xid_);
    (eved)->installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_is::e_memlist_delete(int argc, const char* argv[])
{
// <is> memberlist delete
//
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc>3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be memberlist delete",
      -1));
  return TCL_ERROR;
  }
try
  {
  DeleteISModullist();
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    (eved)->installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_is::e_status(int argc, const char* argv[])
//status
//
// return:
//   id enabled members {triggerlist}
//
{
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_isstatus)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be status", -1));
  return TCL_ERROR;
  }
try
  {
  if (confmode_==synchron)
    {
    C_isstatus* status=ISStatus();
    ostringstream ss;
    (eved)->ff_isstatus(ss, status);
    delete status;
    Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    ISStatus();
    confback.xid(last_xid_);
    (eved)->installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_is::e_enable(int argc, const char* argv[])
{
//enable
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be enable 0|1", -1));
  return TCL_ERROR;
  }
int enab;
if (Tcl_GetInt(interp, argv[2], &enab)!=TCL_OK)  return TCL_ERROR;
try
  {
  enable(enab);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    (eved)->installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_is::e_readoutlist(int argc, const char* argv[])
{
// <IS> readoutlist create|delete|upload
//
int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
  return TCL_ERROR;
  }
else
  {
  static const char* names[]={
    "create",
    "delete",
    "upload",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_rolist_create(argc, argv); break;
    case 1: res=e_rolist_delete(argc, argv); break;
    case 2: res=e_rolist_upload(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/

int E_is::e_rolist_upload(int argc, const char* argv[])
{
// <IS> readoutlist upload trig
//
E_ved::confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, eved, argc, argv, &E_ved::f_readoutlist)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be readoutlist upload trig",
      -1));
  return TCL_ERROR;
  }
int trigg;
if (Tcl_GetInt(interp, argv[3], &trigg)!=TCL_OK)  return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    C_confirmation* conf=UploadReadoutlist(trigg);
    ostringstream ss;
    (eved)->f_readoutlist(ss, conf, 0, 0, 0);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    UploadReadoutlist(trigg);
    confback.xid(last_xid_);
    (eved)->installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/
int E_is::e_rolist_setup(int argc, const char* argv[])
{
// name1 {arg1 ...} name2 {arg1 ...} ...
// called from  E_is::e_rolist_create

    int res=TCL_OK;
    ro_clear();

    if (argc%2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args in E_is::e_rolist_setup "
            "(internal program error)", -1));
        return TCL_ERROR;
    }
    for (int i=0; (res==TCL_OK) && (i<argc); i+=2) {
        const char* proc=argv[i];
        const char* args=argv[i+1];

        const char** mlv;
        int mlc;
        if (Tcl_SplitList(interp, args, &mlc, &mlv)!=TCL_OK)
            return TCL_ERROR;
        try {
            ro_add_proc(proc);
        } catch (C_error* e) {
            Tcl_SetResult_Err(interp, e);
            delete e;
            res=TCL_ERROR;
        }
        for (int j=0; (res==TCL_OK) && (j<mlc); j++) {
            if (mlv[j][0]=='\'') { // it is a string
                char* s=(char*)mlv[j];
                char* se=s+strlen(s)-1;
                if (*se!='\'') {
                    ostringstream ss;
                    ss << "Unmatched ' in " << s;
                    Tcl_SetResult_Stream(interp, ss);
                    res=TCL_ERROR;
                } else {
                    *se=0;
                    try {
                        ro_add_param(s+1);
                    } catch (C_error* e) {
                        Tcl_SetResult_Err(interp, e);
                        delete e;
                        res=TCL_ERROR;
                    }
                    *se='\'';
                }
            } else {
                long arg;
                if (xTclGetLong(interp, (char*)mlv[j], &arg)!=TCL_OK) {
                    res=TCL_ERROR;
                    break;
                }
                try {
                    ro_add_param((int)arg);
                }catch (C_error* e) {
                    Tcl_SetResult_Err(interp, e);
                    delete e;
                    res=TCL_ERROR;
                }
            }
        }
        Tcl_Free((char*)mlv);
    }
    if (res!=TCL_OK)
        ro_clear();

    return res;
}
/*****************************************************************************/
int E_is::e_rolist_create(int argc, const char* argv[])
{
// <IS> readoutlist create prior {triggs} {name1 {arg1 ...} ...}
// <IS> readoutlist create prior {triggs} name1 {arg1 ...} name2 {arg1 ...} ...
//
    E_ved::confbackbox confback;
    int prior;
    ems_u32* trigg;
    int num_trigg;

    if (confmode_==asynchron) {
        if (confback.init(interp, eved, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if ((argc<5) || ((argc>6) && (argc%2==0))) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be "
                "readoutlist create prior "
                "{triggs} {name1 {arg1 ...} name2 {arg1 ...} ...} or "
                "readoutlist create prior "
                "{triggs} name1 {arg1 ...} name2 {arg1 ...} ...",
                -1));
            return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[3], &prior)!=TCL_OK)
        return TCL_ERROR;

    { // read trigger list
        const char** mlv;
        int res=TCL_OK;
        if (Tcl_SplitList(interp, argv[4], &num_trigg, &mlv)!=TCL_OK)
            return TCL_ERROR;
        trigg=new ems_u32[num_trigg];
        for (int i=0; i<num_trigg; i++) {
            if (Tcl_GetInt_u32(interp, mlv[i], trigg+i)!=TCL_OK) {
                res=TCL_ERROR;
            }
        }
        Tcl_Free((char*)mlv);
        if (res!=TCL_OK) {
            delete[] trigg;
            return TCL_ERROR;
        }
    }

    int res;
    if (argc==6) { // procedures are given as TCL list
        const char** mlv;
        int mlc;
        if (Tcl_SplitList(interp, argv[5], &mlc, &mlv)!=TCL_OK) {
            res=-1;
        } else {
            res=e_rolist_setup(mlc, mlv);
            Tcl_Free((char*)mlv);
        }
    } else {       // procedures are given as pairs of proc and args
        res=e_rolist_setup(argc-5, argv+5);
    }

    if (res!=TCL_OK) {
        delete[] trigg;
        return res;
    }

    try {
        DownloadReadoutlist(prior, trigg, num_trigg);
        delete[] trigg;
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            (eved)->installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
        delete[] trigg;
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*****************************************************************************/
#if 0
int E_is::e_rolist_create(int argc, const char* argv[])
{
// <IS> readoutlist create prior {triggs} {name1 {arg1 ...} ...}
// <IS> readoutlist create prior {triggs} name1 {arg1 ...} name2 {arg1 ...} ...
//
    E_ved::confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, eved, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if ((argc<5) || ((argc>6) && (argc%2==0))) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be "
                "readoutlist create prior "
                "{triggs} {name1 {arg1 ...} name2 {arg1 ...} ...} or "
                "readoutlist create prior "
                "{triggs} name1 {arg1 ...} name2 {arg1 ...} ...",
                -1));
            return TCL_ERROR;
    }

    int prior;
    if (Tcl_GetInt(interp, argv[3], &prior)!=TCL_OK)
        return TCL_ERROR;

    ems_u32* trigg;
    int num_trigg;
    const char** mlv;
    if (Tcl_SplitList(interp, argv[4], &num_trigg, &mlv)!=TCL_OK)
        return TCL_ERROR;
    trigg=new ems_u32[num_trigg];
    for (int i=0; i<num_trigg; i++) {
        if (Tcl_GetInt_u32(interp, mlv[i], trigg+i)!=TCL_OK) {
            Tcl_Free((char*)mlv);
            delete[] trigg;
            return TCL_ERROR;
        }
    }
    Tcl_Free((char*)mlv);

    if (argc==6) { // liste
        const char** mlv0;
        int mlc0;
        if (Tcl_SplitList(interp, argv[5], &mlc0, &mlv0)!=TCL_OK)
            return TCL_ERROR;
        if (mlc0%2!=0) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("argument badly formatted", -1));
            Tcl_Free((char*)mlv0);
            delete[] trigg;
            return TCL_ERROR;
        }
        int num_proc=mlc0/2;
        const char** mlv=0;
        int mlc;
        try {
            for (int i=0; i<num_proc; i++) {
                //cerr<<"call ro_add_proc(char* "<<mlv0[i*2]<<")"<<endl;
                ro_add_proc(mlv0[i*2]);
                if (Tcl_SplitList(interp, mlv0[i*2+1], &mlc, &mlv)!=TCL_OK) {
                    Tcl_Free((char*)mlv0);
                    return TCL_ERROR;
                }
                for (int j=0; j<mlc; j++) {
                    long arg;
                    if (xTclGetLong(interp, (char*)mlv[j], &arg)!=TCL_OK) {
                        Tcl_Free((char*)mlv0);
                        Tcl_Free((char*)mlv);
                        delete[] trigg;
                        return TCL_ERROR;
                    }
                    //cerr<<"call ro_add_param(int "<<arg<<")"<<endl;
                    ro_add_param((int)arg);
                }
                free(mlv);
                mlv=0;
            }
        } catch(C_error* e) {
            Tcl_Free((char*)mlv0);
            Tcl_Free((char*)mlv);
            delete[] trigg;
            Tcl_SetResult_Err(interp, e);
            delete e;
            return TCL_ERROR;
        }
        Tcl_Free((char*)mlv0);
    } else { // einzelargumente
        int num_proc=(argc-5)/2;
        const char** mlv=0;
        int mlc;
        try {
            for (int i=0; i<num_proc; i++) {
                //cerr<<"call (2)ro_add_proc(char* "<<argv[i*2+5]<<")"<<endl;
                ro_add_proc(argv[i*2+5]);
                if (Tcl_SplitList(interp, argv[i*2+6], &mlc, &mlv)!=TCL_OK)
                    return TCL_ERROR;
                for (int j=0; j<mlc; j++) {
                    long arg;
                    if (xTclGetLong(interp, (char*)mlv[j], &arg)!=TCL_OK) {
                        Tcl_Free((char*)mlv);
                        delete[] trigg;
                        return TCL_ERROR;
                    }
                    //cerr<<"call (2)ro_add_param(int "<<arg<<")"<<endl;
                    ro_add_param((int)arg);
                }
                Tcl_Free((char*)mlv);
                mlv=0;
            }
        } catch(C_error* e) {
            Tcl_Free((char*)mlv);
            delete[] trigg;
            Tcl_SetResult_Err(interp, e);
            delete e;
            return TCL_ERROR;
        }
    }

    try {
        DownloadReadoutlist(prior, trigg, num_trigg);
        delete[] trigg;
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            (eved)->installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
        delete[] trigg;
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    return TCL_OK;
}
#endif
/*****************************************************************************/
int E_is::e_rolist_delete(int argc, const char* argv[])
{
// <IS> readoutlist delete {triggs}
// <IS> readoutlist delete trigg1 trigg2 ...
//
    E_ved::confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, eved, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc<4) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be readoutlist delete"
                    " trig ... or\n"
                    "  readoutlist delete {trig ...}",
                    -1));
        return TCL_ERROR;
    }

    int* trigg;
    int num;
    if (argc==4) { // triggerliste
        const char** mlv;
        if (Tcl_SplitList(interp, argv[3], &num, &mlv)!=TCL_OK)
            return TCL_ERROR;
        trigg=new int[num];
        for (int i=0; i<num; i++) {
            if (Tcl_GetInt(interp, mlv[i], trigg+i)!=TCL_OK) {
                Tcl_Free((char*)mlv);
                delete[] trigg;
                return TCL_ERROR;
            }
        }
        Tcl_Free((char*)mlv);
    } else { // einzelargumente
        num=argc-3;
        trigg=new int[num];
        for (int i=0; i<num; i++)
            if (Tcl_GetInt(interp, argv[3+i], trigg+i)!=TCL_OK) {
            delete[] trigg;
            return TCL_ERROR;
        }
    }

    int res;
    try {
        if (confmode_==synchron) {
            for (int i=0; i<num; i++) DeleteReadoutlist(trigg[i]);
            res=TCL_OK;
        } else {
            if (num==1) {
                DeleteReadoutlist(trigg[0]);
                confback.xid(last_xid_);
                (eved)->installconfback(confback);
                ostringstream ss;
                ss << last_xid_;
                Tcl_SetResult_Stream(interp, ss);
                res=TCL_OK;
            } else {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj( "in asynchronous mode exactly one argument required",
                    -1));
                res=TCL_ERROR;
            }
        }
        delete[] trigg;
    } catch(C_error* e) {
        delete[] trigg;
        Tcl_SetResult_Err(interp, e);
        delete e;
        res=TCL_ERROR;
    }
    return res;
}

/*****************************************************************************/
/*****************************************************************************/
