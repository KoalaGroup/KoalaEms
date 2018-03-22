/*
 * emstcl_vedcomm.cc
 * created: 07.09.95 PW
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "emstcl_ved.hxx"
#include <ems_errors.hxx>
#include <ved_errors.hxx>
#include <inbuf.hxx>
#include <outbuf.hxx>
#include <objecttypes.h>
#include <reqstrings.h>
#include "emstcl_is.hxx"
#include "emstcl.hxx"
#include "findstring.hxx"
#include "tcl_cxx.hxx"
#include "versions.hxx"

VERSION("2016-Jan-15", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl_vedcomm.cc,v 1.53 2016/05/02 15:32:58 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

int E_ved::e_close(int argc, const char* argv[])
//close
{
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be close", -1));
  return TCL_ERROR;
  }
//cerr << "E_ved::e_close(" << (void*)this << ")" << endl;
Tcl_DeleteCommand(interp, argv[0]);
return TCL_OK;
}

/*****************************************************************************/

int E_ved::e_closecommand(int argc, const char* argv[])
//closecommand
// wird kurz vor $ved close und rename $ved {} ausgefuehrt
// %n wird durch realen VEDnamen ersetzt
// %c wird durch Kommandonamen ersetzt
{
if (argc>3)
  {
  Tcl_SetObjResult(interp,
        Tcl_NewStringObj("wrong # args; must be closecommand [command]", -1));
  return TCL_ERROR;
  }
Tcl_SetResult_String(interp, ccomm);
ccomm=argc==3?argv[2]:"";
return TCL_OK;
}

/*****************************************************************************/

int E_ved::e_name(int argc, const char* argv[])
//name
{
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be name", -1));
  return TCL_ERROR;
  }
Tcl_SetResult_String(interp, name());
return TCL_OK;
}

/******************************************************************************
*
* confmode legt den Confirmationmodus eines VED fest. 
* Rueckgabe ist der vorherige Modus.
*
*/

int E_ved::e_confmode(int argc, const char* argv[])
{
// <VED> confmode [synchron|asynchron]
if (argc>3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be confmode [synchron|asynchron]", -1));
  return TCL_ERROR;
  }
conf_mode oldmode;
if (argc==2)
  {
  oldmode=confmode();
  }
else
  {
  conf_mode mode;
  const char* names[]={
    "synchron",
    "asynchron",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: mode=synchron; break;
    case 1: mode=asynchron; break;
    default: return TCL_ERROR; break;
    }
  oldmode=confmode(mode);
  }
Tcl_SetObjResult(interp,
    Tcl_NewStringObj((char*)(oldmode==synchron?"synchron":"asynchron"), -1));
return TCL_OK;
}

/******************************************************************************
*
* pending stellt fest, wieviele Confirmations noch fuer dieses VED
* erwartet werden.
*/

int E_ved::e_pending(int argc, const char* argv[])
{
// <VED> pending
if (argc>2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be pending", -1));
  return TCL_ERROR;
  }
ostringstream st;
st << confbacknum();
Tcl_SetResult_Stream(interp, st);
return TCL_OK;
}

/*****************************************************************************/

void E_ved_timerdummy(ClientData)
{
cerr << "E_ved::timerdummy called" << endl;
}

/*****************************************************************************/

int E_ved::hasconfback(int xid) const
{
for (int i=0; i<numconfbacks; i++)
  if (confbacks[i]->xid==xid) return 1;
return 0;
}

/******************************************************************************
*
* flush bis alle Confirmations eingesammelt sind, bricht
* jedoch nach Ablauf eines Timeouts ab. Die Anzahl der noch ausstehenden
* Confirmations wird zurueckgegeben.
* Wenn xid_list angegeben ist, wird nur gewartet bis alle Confirmations mit
* den angegebenen Transaction-IDs erhalten wurden. Bei Timeout wird die Liste
* der noch fehlenden IDs zurueckgegeben.
*/

int E_ved::e_flush(int argc, const char* argv[])
{
// <VED> flush [timeout [xid_list]]
    if (argc>4) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be flush [timeout/ms [xid_list]]",
                -1));
        return TCL_ERROR;
    }

    struct timeval start, jetzt;
    int timeout=0, msec;
    Tcl_TimerToken token=0;
    int res;
    if ((argc>2) && (Tcl_GetInt(interp, argv[2], &timeout)!=TCL_OK))
        return TCL_ERROR;

    if (timeout) {
        gettimeofday(&start, 0);
        token=Tcl_CreateTimerHandler(timeout, E_ved_timerdummy, 0);
    }

    if (argc>3) {
        int lc, *ilv, i;
        const char** lv;
        if (Tcl_SplitList(interp, argv[3], &lc, &lv)!=TCL_OK) {
            res=TCL_ERROR;
            goto raus;
        }

        ilv=new int[lc];
        for (i=0; i<lc; i++) {
            if (Tcl_GetInt(interp, lv[i], ilv+i)!=TCL_OK) {
                delete[] ilv;
                Tcl_Free((char*)lv);
                res=TCL_ERROR;
                goto raus;
            }
        }
        Tcl_Free((char*)lv);

        int num, nochzeit=1;
        num=0;
        for (i=0; i<lc; i++)
            if (hasconfback(ilv[i])) num++;
        while (num && nochzeit) {
            Tcl_DoOneEvent(TCL_ALL_EVENTS);
            num=0;
            for (i=0; i<lc; i++)
                if (hasconfback(ilv[i])) num++;
            if (timeout) {
                gettimeofday(&jetzt, 0);
                msec=(jetzt.tv_usec-start.tv_usec)/1000+(jetzt.tv_sec-start.tv_sec)*1000;
                nochzeit=msec<timeout;
            }
        }

        for (i=0; i<lc; i++) {
            if (hasconfback(ilv[i])) {
                ostringstream st;
                st << ilv[i];
                Tcl_AppendElement_Stream(interp, st);
            }
        }
        res=TCL_OK;
    } else {
        int num, nochzeit=1;
        while (((num=confbacknum())>0) && (nochzeit)) {
            Tcl_DoOneEvent(TCL_ALL_EVENTS);
            if (timeout) {
                gettimeofday(&jetzt, 0);
                msec=(jetzt.tv_usec-start.tv_usec)/1000+(jetzt.tv_sec-start.tv_sec)*1000;
                nochzeit=msec<timeout;
            }
        }
        ostringstream st;
        st << num;
        Tcl_SetResult_Stream(interp, st);
        res=TCL_OK;
    }
    raus:
    if (timeout)
        Tcl_DeleteTimerHandler(token);
    return res;
}
/*****************************************************************************/

int E_ved::e_reset(int argc, const char* argv[])
{
//reset
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be reset", -1));
  return TCL_ERROR;
  }
try
  {
  ResetVED();
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_var(int argc, const char* argv[])
{
// <VED> var create|delete|read|write|size
//

int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
  res=TCL_ERROR;
  }
else
  {
  const char* names[]={
    "create",
    "delete",
    "read",
    "write",
    "size",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_var_create(argc, argv); break;
    case 1: res=e_var_delete(argc, argv); break;
    case 2: res=e_var_read(argc, argv); break;
    case 3: res=e_var_write(argc, argv); break;
    case 4: res=e_var_size(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/

int E_ved::e_var_create(int argc, const char* argv[])
//var create idx [size]
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if ((argc!=4) && (argc!=5))
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be var create idx [size]", -1));
  return TCL_ERROR;
  }
int idx, size;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
size=1;
if ((argc==5) && (Tcl_GetInt(interp, argv[4], &size)!=TCL_OK)) return TCL_ERROR;
try
  {
  CreateVariable(idx, size);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_var_delete(int argc, const char* argv[])
//var delete idx
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be var delete idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  DeleteVariable(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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
int E_ved::e_var_read(int argc, const char* argv[])
{
// <VED> var read idx [ <fist_idx> [<num>]]
    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_readvar)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc>6) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be var read idx [<first> [<num>]]",
                -1));
        return TCL_ERROR;
    }
    int idx, first, num, last=0;
    if (confmode_==synchron) {
        first=0; last=-1;
        if (argc>4) {
            if (Tcl_GetInt(interp, argv[4], &first)!=TCL_OK) return TCL_ERROR;
        }
        if (argc>5) {
            if (Tcl_GetInt(interp, argv[5], &num)!=TCL_OK) return TCL_ERROR;
            last=first+num;
        }
    } else {
        if ((argc>4) && (confback.isimplicit())) {
            if (argc>5) confback.formargs(1, argv[5]);
            confback.formargs(0, argv[4]);
        }
    }
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
    try {
        if (confmode_==synchron) {
            ems_i32* arr;
            int size;
            ReadVariable(idx, &arr, &size);
            if (last==-1) last=size;
            if (last>size) {
                delete[] arr;
                ostringstream ss;
                ss << "Variable "<< idx << " has only " << size << " elements";
                Tcl_SetResult_Stream(interp, ss);
                return TCL_ERROR;
            }
            for (int i=first; i<last; i++) {
                ostringstream ss;
                ss << arr[i];
                Tcl_AppendElement_Stream(interp, ss);
            }
            delete[] arr;
        } else {
            ReadVariable(idx);
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* error) {
        Tcl_SetResult_Err(interp, error);
        delete error;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*****************************************************************************/
int E_ved::e_var_write(int argc, const char* argv[])
//var write idx {args}
{
    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc<5) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be var write idx list|values",
            -1));
        return TCL_ERROR;
    }

    int idx, num;
    ems_i32* arr;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK)
        return TCL_ERROR;
    if (argc==5) { // argv[4] ist Liste
        const char** lv;
        if (Tcl_SplitList(interp, argv[4], &num, &lv)!=TCL_OK)
            return TCL_ERROR;

        arr=new ems_i32[num];
        for (int i=0; i<num; i++) {
            long x;
            /*if (Tcl_GetInt(interp, lv[i], arr+i)!=TCL_OK)*/
            if (xTclGetLong(interp, (char*)lv[i], &x)!=TCL_OK) {
                delete[] arr;
                Tcl_Free((char*)lv);
                return TCL_ERROR;
            } else {
                arr[i]=x;
            }
        }
        Tcl_Free((char*)lv);
    } else { // argv[4] argv[5] ... sind Zahlen
        num=argc-4;
        arr=new ems_i32[num];
        for (int i=0; i<num; i++) {
            long x;
            /*if (Tcl_GetInt(interp, argv[4+i], arr+i)!=TCL_OK)*/
            if (xTclGetLong(interp, (char*)argv[4+i], &x)!=TCL_OK) {
                delete[] arr;
                return TCL_ERROR;
            } else {
                arr[i]=x;
            }
        }
    } try {
        WriteVariable(idx, arr, num);
        delete[] arr;
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* error) {
        delete[] arr;
        Tcl_SetResult_Err(interp, error);
        delete error;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*****************************************************************************/

int E_ved::e_var_size(int argc, const char* argv[])
//var size idx
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_integer)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be var size idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    int size=GetVariableAttributes(idx);
    ostringstream ss;
    ss << size;
    Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    GetVariableAttributes(idx);
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_identify(int argc, const char* argv[])
{
// <VED> identify [level]
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_identify)!=TCL_OK)
      return TCL_ERROR;
  }
else
  {
  if (confback.finit(interp, this, argc, argv, &E_ved::f_identify)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc>3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be identify [level]", -1));
  return TCL_ERROR;
  }
int level=2;
if ((argc==3) && (Tcl_GetInt(interp, argv[2], &level)!=TCL_OK))
    return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=Identify(level);
    ostringstream ss;
    f_identify(ss, conf, 0, 0, 0);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    Identify(level);
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_initiate(int argc, const char* argv[])
{
// <VED> initiate id
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_rawintegerlist)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be initiate id", -1));
  return TCL_ERROR;
  }
int id;
if (Tcl_GetInt(interp, argv[2], &id)!=TCL_OK) return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    C_confirmation* conf;
    conf=Initiate(id);
    ostringstream ss;
    f_rawintegerlist(ss, conf, 0, 0, 0);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    Initiate(id);
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_status(int argc, const char* argv[])
{
// <VED> status
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_integer)!=TCL_OK)
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
    C_confirmation* conf;
    conf=GetVEDStatus();
    ostringstream ss;
    f_integer(ss, conf, 0, 0, 0);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    GetVEDStatus();
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_namelist(int argc, const char* argv[])
{
// moegliche Argumente:
//
//  keine           : sollte Baum aller Objekte liefern
//  "null"|0        : liefert Liste aller Objekttypen
//  Objekttyp       : liefert Liste aller Objekte dieses Types
//                    oder Fehler, falls es Untertypen gibt
//  Objekttyp null|0: liefert Liste aller Objekte dieses Untertyps
//  beliebige ints  : ...
//

confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_integerlist)!=TCL_OK)
      return TCL_ERROR;
  }

int size=argc-2;
int* arr;
arr=new int[size];
if (size>=1)
  {
  const char* onames[]={
    "null",
    "ved",
    "domain",
    "is",
    "var",
    "pi",
    "do",
    0};
  switch (findstring(interp, argv[2], onames, arr))
    {
    case 0:
      arr[0]=Object_null; break;
    case 1:
      arr[0]=Object_ved; break;
    case 2:
      arr[0]=Object_domain; break;
    case 3:
      arr[0]=Object_is; break;
    case 4:
      arr[0]=Object_var; break;
    case 5:
      arr[0]=Object_pi; break;
    case 6:
      arr[0]=Object_do; break;
    case -2:
      // war integer
      break;
    default:
      delete[] arr;
      return TCL_ERROR;
    }
  }
if (size>=2)
  {
  switch (arr[0])
    {
    case Object_domain:
      {
      const char* dnames[]={
        "null",
        "ml",
        "lam",
        "trig",
        "event",
        "di",
        "do",
        0};
      switch (findstring(interp, argv[3], dnames, arr+1))
        {
        case 0:
          arr[1]=Dom_null; break;
        case 1:
          arr[1]=Dom_Modullist; break;
        case 2:
          arr[1]=Dom_LAMproclist; break;
        case 3:
          arr[1]=Dom_Trigger; break;
        case 4:
          arr[1]=Dom_Event; break;
        case 5:
          arr[1]=Dom_Datain; break;
        case 6:
          arr[1]=Dom_Dataout; break;
        case -2:
          // war integer
          break;
        default:
          delete[] arr;
          return TCL_ERROR;
        }
      }
      break;
    case Object_pi:
      {
      const char* pnames[]={
        "null",
        "ro",
        "lam",
        0};
      switch (findstring(interp, argv[3], pnames, arr+1))
        {
        case 0:
          arr[1]=Invocation_null; break;
        case 1:
          arr[1]=Invocation_readout; break;
        case 2:
          arr[1]=Invocation_LAM; break;
        case -2:
          // war integer
          break;
        default:
          delete[] arr;
          return TCL_ERROR;
        }
      }
      break;
    default:
      if (Tcl_GetInt(interp, argv[3], arr+1)!=TCL_OK)
        {
        delete[] arr;
        return TCL_ERROR;
        }
      break;
    }
  }
for (int i=2; i<size; i++)
  {
  if (Tcl_GetInt(interp, argv[i+2], arr+i)!=TCL_OK)
    {
    delete[] arr;
    return TCL_ERROR;
    }
  }
try
  {
  if (confmode_==synchron)
    {
    C_confirmation* conf=GetNamelist(arr, size);
    ostringstream ss;
    f_integerlist(ss, conf, 0, 0, 0);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    GetNamelist(arr, size);
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  delete[] arr;
  }
catch(C_error* e)
  {
  delete[] arr;
  Tcl_SetResult_Err(interp, e);
  delete e;
  return TCL_ERROR;
  }
return TCL_OK;
}

/******************************************************************************
*
* <VED> typedunsol installiert eine Callbackfunktion fuer unsolicited Messages
*
* argv[3]: TCL_Kommando
*          %h wird durch den Header, %d durch die Daten,
*          %v durch das VED-Kommando ersetzt
*/

int E_ved::e_typedunsol(int argc, const char* argv[])
//<VED> typedunsol type [command]
{
    if ((argc<3) || (argc>4)) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be typedunsol type [Command]",
            -1));
        return TCL_ERROR;
    }

    int type, res;
    res=findstring(interp, argv[2], unsolstrs, &type);
    switch (res) {
    case -1: // nicht gefunden
        return TCL_ERROR;
    case -2: // war integer
        break;
    default:
        type=res;
        break;
    }

    Tcl_SetResult_String(interp, unsolcomm[type]);
    if (argc==3)
        unsolcomm[type]="";
    else
        unsolcomm[type]=argv[3];
    return TCL_OK;
}
/******************************************************************************
*
* <VED> unsol installiert eine Callbackfunktion fuer unsolicited Messages
*
* argv[3]: TCL_Kommando
*          %h wird durch den Header, %d durch die Daten,
*          %v durch das VED-Kommando ersetzt
*/

int E_ved::e_unsol(int argc, const char* argv[])
//<VED> unsol [command]
{
if ((argc<2) || (argc>3))
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be unsol [Command]", -1));
  return TCL_ERROR;
  }
Tcl_SetResult_String(interp, defunsolcomm);
if (argc==2)
  {
  SetUnsol(0);
  defunsolcomm="";
  }
else
  {
  defunsolcomm=argv[2];
  SetUnsol(1);
  }
return TCL_OK;
}

/*****************************************************************************/

int E_ved::e_event(int argc, const char* argv[])
{
// <VED> event [max]
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_domevent)!=TCL_OK)
      return TCL_ERROR;
  }
else
  {
  if (confback.finit(interp, this, argc, argv, &E_ved::f_domevent)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc>3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be event [max]", -1));
  return TCL_ERROR;
  }
try
  {
  if (confmode_==synchron)
    {
    C_confirmation* conf=UploadEvent();
    ostringstream ss;
    if (argc>2)
      {
      string st(argv[2]);
      f_domevent(ss, conf, 0, 1, &st);
      }
    else
      //f_domevent(ss, conf, 0, 0, 0);
      confback.callformer(this, ss, conf);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    UploadEvent();
    confback.xid(last_xid_);
    if ((argc>2) && (confback.isimplicit())) confback.formargs(0, argv[2]);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_readout(int argc, const char* argv[])
{
// <VED> readout start|stop|resume|reset|status
//

int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
  res=TCL_ERROR;
  }
else
  {
  const char* names[]={
    "start",
    "stop",
    "resume",
    "reset",
    "status",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_ro_start(argc, argv); break;
    case 1: res=e_ro_stop(argc, argv); break;
    case 2: res=e_ro_resume(argc, argv); break;
    case 3: res=e_ro_reset(argc, argv); break;
    case 4: res=e_ro_status(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/

int E_ved::e_ro_start(int argc, const char* argv[])
//readout start
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be readout start", -1));
  return TCL_ERROR;
  }
try
  {
  StartReadout();
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_ro_stop(int argc, const char* argv[])
//readout stop
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be readout stop", -1));
  return TCL_ERROR;
  }
try
  {
  StopReadout();
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_ro_resume(int argc, const char* argv[])
//readout resume
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be readout resume", -1));
  return TCL_ERROR;
  }
try
  {
  ResumeReadout();
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_ro_reset(int argc, const char* argv[])
//readout reset
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be readout reset", -1));
  return TCL_ERROR;
  }
try
  {
  ResetReadout();
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
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

int E_ved::e_ro_status(int argc, const char* argv[])
{
// <VED> readout status [aux ...]
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_readoutstatus)!=TCL_OK)
      return TCL_ERROR;
  }
else
  { // wirkt noch nicht
  if (confback.finit(interp, this, argc, argv, &E_ved::f_readoutstatus)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc<3)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be readout status [aux ...]", -1));
  return TCL_ERROR;
  }
int* aux;
aux=new int[argc-3];
for (int i=3; i<argc; i++)
  {
  if (Tcl_GetInt(interp, argv[i], aux+i-3)!=TCL_OK)
    {
    delete[] aux;
    return TCL_ERROR;
    }
  }
try
  {
  if (confmode_==synchron)
    {
    C_readoutstatus* rostatus;
    ostringstream ss;
    rostatus=GetReadoutStatus(argc-3, aux);
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
    Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    GetReadoutStatus(argc-3, aux);
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  delete[] aux;
  }
catch(C_error* error)
  {
  Tcl_SetResult_Err(interp, error);
  delete error;
  delete[] aux;
  return TCL_ERROR;
  }
return TCL_OK;
}

/*****************************************************************************/

int E_ved::e_dataout_upload(int argc, const char* argv[])
{
// <VED> dataout upload idx
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_ioaddr)!=TCL_OK)
      return TCL_ERROR;
  }
int res;
if (argc!=4)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args, must be dataout upload idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    C_data_io* addr=UploadDataout(idx);
    ostringstream ss;
    res=ff_ioaddr(ss, addr);
    delete addr;
    if (res==TCL_OK) Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    UploadDataout(idx);
    if (confback.isimplicit()) confback.formargs(0, "out");
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    res=TCL_OK;
    }
  }
catch(C_error* e)
  {
  res=TCL_ERROR;
  Tcl_SetResult_Err(interp, e);
  delete e;
  }
return res;
}

/*****************************************************************************/
/*****************************************************************************/

int E_ved::e_datain(int argc, const char* argv[])
// <VED> datain {create delete upload}
{
int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
  res=TCL_ERROR;
  }
else
  {
  const char* names[]={
    "create",
    "delete",
    "upload",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_datain_create(argc, argv); break;
    case 1: res=e_datain_delete(argc, argv); break;
    case 2: res=e_datain_upload(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/
int E_ved::e_datain_create(int argc, const char* argv[])
{
// old:
// <VED> datain create idx ringbuffer|stream|cluster addr
// addr: raw   address
//       modul name
//       driver_(mixed|mapped|syscall) address [space [offset [option]]]
// new:
// <VED> datain create idx {buffer} {address}
//   buffer : ringbuffer|stream|cluster|opaque|mqtt arg ...
//   address: addrtype arg ...
//
    const char* ionames[]={
        "ringbuffer",
        "stream",
        "cluster",
        "opaque",
        "mqtt",
        0};
    const char* addrnames[]={
        "raw",
        "modul",
        "driver_mapped",
        "driver_mixed",
        "driver_syscall",
        "socket",
        "localsocket",
        "file",
        "tape",
        "null",
        "asynchfile",
        "autosocket",
        "v6socket",
        0
    };
    confbackbox confback;

    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc<6) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be datain create idx buffertype "
                "addresstype address or\n"
                "datain create idx {buffer} {address}",
                -1));
        return TCL_ERROR;
    }

    int idx, res;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK)
        return TCL_ERROR;

    int uselists=argc==6;
    int blc, alc, *blvi=0;
    const char **blv, **alv;
    C_io_type* typ=0;
    C_io_addr* addr=0;
    C_data_io* io_addr=0;

    if (uselists) {
        if (Tcl_SplitList(interp, argv[4], &blc, &blv)!=TCL_OK)
            return TCL_ERROR;
        if (Tcl_SplitList(interp, argv[5], &alc, &alv)!=TCL_OK) {
            Tcl_Free((char*)blv);
            return TCL_ERROR;
        }
        if (blc==0) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("Bufferlist is empty", -1));
            goto fehler;
        }
        if (alc==0) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("Addresslist is empty", -1));
            goto fehler;
        }
    } else {
        blc=1;
        blv=argv+4;
        alc=argc-5;
        alv=argv+5;
    }
#if 0
    blvi=new int[blc-1];

    int i;
    for (i=0; i<blc-1; i++) {
        long x;
        /*if (Tcl_GetInt(interp, blv[i+1], blvi+i)!=TCL_OK) goto fehler;*/
        if (xTclGetLong(interp, (char*)blv[i+1], &x)!=TCL_OK)
            goto fehler;
        else
            blvi[i]=x;
    }

    switch (findstring(interp, blv[0], ionames)) {
    case 0:
        typ=new C_io_type_ring(io_in, blc-1, blvi);
        break;
    case 1:
        typ=new C_io_type_stream(io_in, blc-1, blvi);
        break;
    case 2:
        typ=new C_io_type_cluster(io_in, blc-1, blvi);
        break;
    case 3:
        typ=new C_io_type_opaque(io_in, blc-1, blvi);
        break;
    case 4:
        typ=new C_io_type_mqtt(io_in, blc-1, blvi);
        break;
    default:
        goto fehler;
    }
#else
    {
        C_outbuf blva;
#if 0
    for (int x=0; x<blc; x++) {
        cerr<<"blv["<<x<<"]="<<blv[x]<<endl;
    }
#endif
        for (int i=1; i<blc; i++) {
            if (blv[i][0]=='\'') {
                const char *s=blv[i]+1;
                int l=strlen(s);
                if (s[l-1]!='\'') {
                    ostringstream ss;
                    ss << "Unmatched ' in " << s;
                    Tcl_SetResult_Stream(interp, ss);
                    goto fehler;
                }
                blva.putchararr(s, l-1);
            } else {
                long x;
                if (xTclGetLong(interp, (char*)blv[i], &x)!=TCL_OK)
                    goto fehler;
                else
                    blva<<static_cast<uint32_t>(x);
            }
        }

        switch (findstring(interp, blv[0], ionames)) {
        case 0:
            typ=new C_io_type_ring(io_in, blva);
            break;
        case 1:
            typ=new C_io_type_stream(io_in, blva);
            break;
        case 2:
            typ=new C_io_type_cluster(io_in, blva);
            break;
        case 3:
            typ=new C_io_type_opaque(io_in, blva);
            break;
        case 4:
            typ=new C_io_type_mqtt(io_in, blva);
            break;
        default:
            goto fehler;
        }
    }
#endif

    IOAddr addrtyp;
    switch (findstring(interp, alv[0], addrnames)) {
    case 0:
        addrtyp=Addr_Raw; break;
    case 1:
        addrtyp=Addr_Modul; break;
    case 2:
        addrtyp=Addr_Driver_mapped; break;
    case 3:
        addrtyp=Addr_Driver_mixed; break;
    case 4:
        addrtyp=Addr_Driver_syscall; break;
    case 5:
        addrtyp=Addr_Socket; break;
    case 6:
        addrtyp=Addr_LocalSocket; break;
    case 7:
        addrtyp=Addr_File; break;
    case 8:
        addrtyp=Addr_Tape; break;
    case 9:
        addrtyp=Addr_Null; break;
    case 10:
        addrtyp=Addr_AsynchFile; break;
    case 11:
        addrtyp=Addr_Autosocket; break;
    case 12:
        addrtyp=Addr_V6Socket; break;
    default:
        goto fehler;
    }

    try {
        switch (addrtyp) {
        case Addr_Raw: {
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; "
                        "must be ... raw addr",
                        -1));
            goto fehler;
            }
            long address;
            if (xTclGetLong(interp, (char*)alv[1], &address)!=TCL_OK)
                goto fehler;
            addr=new C_io_addr_raw(io_in, address);
        }
        break;
        case Addr_Modul:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... modul name",
                        -1));
                goto fehler;
            }
            addr=new C_io_addr_modul(io_in, alv[1]);
            break;
        case Addr_Driver_mapped:
        case Addr_Driver_mixed:
        case Addr_Driver_syscall:
            if ((alc<2) || (alc>5)) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; "
                    "must be ... driver... name [space [offset [option]]]"
                    , -1));
                goto fehler;
            }
            {
                int space=0, offset=0, option=0;
                if (alc>2)
                    if (Tcl_GetInt(interp, alv[2], &space)!=TCL_OK) goto fehler;
                if (alc>3)
                    if (Tcl_GetInt(interp, alv[3], &offset)!=TCL_OK) goto fehler;
                if (alc>4)
                    if (Tcl_GetInt(interp, alv[4], &option)!=TCL_OK) goto fehler;
                switch (addrtyp) {
                case Addr_Driver_mapped:
                    addr=new C_io_addr_driver_mapped(io_in, alv[1], space,
                            offset, option);
                    break;
                case Addr_Driver_mixed:
                    addr=new C_io_addr_driver_mixed(io_in, alv[1], space,
                            offset, option);
                    break;
                case Addr_Driver_syscall:
                    addr=new C_io_addr_driver_syscall(io_in, alv[1], space,
                            offset, option);
                    break;
                default:
                    cerr<<"emstcl_vedcomm.cc: E_ved::e_datain_create: "
                            "programm error"<<endl;
                    Tcl_SetObjResult(interp,
                        Tcl_NewStringObj("emstcl_vedcomm.cc: E_ved::e_datain_create: "
                            "programm error",
                            -1));
                    goto fehler;
                }
            }
            break;
        case Addr_Socket:
            {
                int port;
                if (alc==2) {
                    if (Tcl_GetInt(interp, alv[1], &port)!=TCL_OK) goto fehler;
                    addr=new C_io_addr_socket(io_in, port);
                } else if (alc==3) {
                    if (Tcl_GetInt(interp, alv[2], &port)!=TCL_OK) goto fehler;
                    addr=new C_io_addr_socket(io_in, alv[1], port);
                } else {
                    Tcl_SetObjResult(interp,
                        Tcl_NewStringObj("wrong # args; must be ... socket [host] port",
                            -1));
                    goto fehler;
                }
            }
            break;
        case Addr_V6Socket:
            {
                if (alc==2) {
                    addr=new C_io_addr_v6socket(io_in, ip_passive, alv[1]);
                } else if (alc==3) {
                    addr=new C_io_addr_v6socket(io_in, ip_default,
                            alv[1], alv[2]);
                } else {
                    Tcl_SetObjResult(interp,
                        Tcl_NewStringObj("wrong # args; must be ... v6socket "
                                "[host] port",
                            -1));
                    goto fehler;
                }
            }
            break;
        case Addr_Autosocket:
            {
                int port;
                if (alc==2) {
                    if (Tcl_GetInt(interp, alv[1], &port)!=TCL_OK) goto fehler;
                    addr=new C_io_addr_autosocket(io_in, port);
                } else {
                    Tcl_SetObjResult(interp,
                        Tcl_NewStringObj("wrong # args; must be ... autosocket port",
                            -1));
                    goto fehler;
                }
            }
            break;
        case Addr_LocalSocket:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... localsocket name",
                        -1));
                goto fehler;
            }
            addr=new C_io_addr_localsocket(io_in, alv[1]);
            break;
        case Addr_File:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... file name",
                        -1));
                goto fehler;
            }
            addr=new C_io_addr_file(io_in, alv[1]);
            break;
        case Addr_AsynchFile:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... file name",
                        -1));
                goto fehler;
            }
            addr=new C_io_addr_asynchfile(io_in, alv[1]);
            break;
        case Addr_Tape:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... tape name",
                        -1));
                goto fehler;
            }
            addr=new C_io_addr_tape(io_in, alv[1]);
            break;
        case Addr_Null:
            if (alc!=1) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... null",
                        -1));
                goto fehler;
            }
            addr=new C_io_addr_null(io_in);
            break;
        }
        io_addr=new C_data_io(typ, addr);
        typ=0; addr=0;
        io_addr->forcelists(uselists);
        DownloadDatain(idx, *io_addr);
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        goto fehler;
    }
    res=TCL_OK;
    goto raus;

fehler:
    res=TCL_ERROR;
raus:
    delete[] blvi;
    if (uselists) {
        Tcl_Free((char*)blv);
        Tcl_Free((char*)alv);
    }
    delete typ;
    delete addr;
    delete io_addr;
    return res;
}
/*****************************************************************************/

int E_ved::e_datain_delete(int argc, const char* argv[])
{
// <VED> datain delete idx
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=4)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be datain delete idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  DeleteDatain(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_datain_upload(int argc, const char* argv[])
{
// <VED> datain upload idx
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_ioaddr)!=TCL_OK)
      return TCL_ERROR;
  }
int res;
if (argc!=4)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args, must be datain upload idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    C_data_io* addr=UploadDatain(idx);
    ostringstream ss;
    res=ff_ioaddr(ss, addr);
    delete addr;
    if (res==TCL_OK) Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    UploadDatain(idx);
    if (confback.isimplicit()) confback.formargs(0, "in");
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    res=TCL_OK;
    }
  }
catch(C_error* e)
 {
  res=TCL_ERROR;
  Tcl_SetResult_Err(interp, e);
  delete e;
  }
return res;
}

/*****************************************************************************/

int E_ved::e_dataout(int argc, const char* argv[])
// <VED> dataout {create|delete|upload|status}
{
int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
  res=TCL_ERROR;
  }
else
  {
  const char* names[]={
    "create",
    "delete",
    "upload",
    "status",
    "wind",
    "write",
    "writefile",
    "enable",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_dataout_create(argc, argv); break;
    case 1: res=e_dataout_delete(argc, argv); break;
    case 2: res=e_dataout_upload(argc, argv); break;
    case 3: res=e_dataout_status(argc, argv); break;
    case 4: res=e_dataout_wind(argc, argv); break;
    case 5: res=e_dataout_write(argc, argv); break;
    case 6: res=e_dataout_writefile(argc, argv); break;
    case 7: res=e_dataout_enable(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/
int E_ved::e_dataout_create(int argc, const char* argv[])
{
// old:
// <VED> dataout create idx ringbuffer|stream|cluster buffersize prior addr
// addr: raw         address
//       modul       name
//       driver_(mixed|mapped|syscall) address [space [offset [option]]]
//       socket      host port
//       localsocket name
//       file        name
//       tape        name
//       null
//
// new:
// <VED> dataout create idx {buffer} {address}
//   buffer : ringbuffer|stream|cluster arg ...
//   address: addrtype arg ...
//
    const char* ionames[]={"ringbuffer", "stream", "cluster", 0};
    const char* addrnames[]={
        "raw",
        "modul",
        "driver_mapped",
        "driver_mixed",
        "driver_syscall",
        "socket",
        "localsocket",
        "file",
        "tape",
        "null",
        "asynchfile",
        "autosocket",
        "v6socket",
        0
    };
    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }
    if (argc<6) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be dataout create idx "
            "buffertype buffersize priority addresstype address\n"
            "or dataout create idx {buffer} {address} [{args}]",
            -1));
        return TCL_ERROR;
    }
    int idx, res;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK)
        return TCL_ERROR;
    int uselists=argc<8;
    int blc, alc, clc, *blvi=0;
    const char **blv, **alv, **clv;
    C_io_type* typ=0;
    C_io_addr* addr=0;
    C_data_io* io_addr=0;

    if (uselists) {
        if (Tcl_SplitList(interp, argv[4], &blc, &blv)!=TCL_OK)
            return TCL_ERROR;
        if (Tcl_SplitList(interp, argv[5], &alc, &alv)!=TCL_OK) {
            Tcl_Free((char*)blv);
            return TCL_ERROR;
        }
        if (argc>6) {
            if (Tcl_SplitList(interp, argv[6], &clc, &clv)!=TCL_OK) {
                Tcl_Free((char*)blv);
                Tcl_Free((char*)alv);
                return TCL_ERROR;
            }
        } else
            clv=0;
        if (blc==0) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("Bufferlist is empty", -1));
            goto fehler;
        }
        if (alc==0) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("Addresslist is empty", -1));
            goto fehler;
        }
    } else {
        blc=3;
        blv=argv+4;
        alc=argc-7;
        alv=argv+7;
        clv=0;
    }

    blvi=new int[blc-1];
    int i;
    for (i=0; i<blc-1; i++) {
        if (Tcl_GetInt(interp, blv[i+1], blvi+i)!=TCL_OK)
            goto fehler;
    }
    switch (findstring(interp, blv[0], ionames)) {
    case 0:
        typ=new C_io_type_ring(io_out, blc-1, blvi);
        break;
    case 1:
        typ=new C_io_type_stream(io_out, blc-1, blvi);
        break;
    case 2:
        typ=new C_io_type_cluster(io_out, blc-1, blvi);
        break;
    default:
        goto fehler;
    }

    IOAddr addrtyp;
    switch (findstring(interp, alv[0], addrnames)) {
    case 0:
        addrtyp=Addr_Raw;
        break;
    case 1:
        addrtyp=Addr_Modul;
        break;
    case 2:
        addrtyp=Addr_Driver_mapped;
        break;
    case 3:
        addrtyp=Addr_Driver_mixed;
        break;
    case 4:
        addrtyp=Addr_Driver_syscall;
        break;
    case 5:
        addrtyp=Addr_Socket;
        break;
    case 6:
        addrtyp=Addr_LocalSocket;
        break;
    case 7:
        addrtyp=Addr_File;
        break;
    case 8:
        addrtyp=Addr_Tape;
        break;
    case 9:
        addrtyp=Addr_Null;
        break;
    case 10:
        addrtyp=Addr_AsynchFile;
        break;
    case 11:
        addrtyp=Addr_Autosocket;
        break;
    case 12:
        addrtyp=Addr_V6Socket;
        break;
    default:
        goto fehler;
    }

    try {
        /*C_io_addr* addr=0;*/
        switch (addrtyp) {
        case Addr_Raw: {
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... raw addr",
                        -1));
                goto fehler;
            }
            int address;
            if (Tcl_GetInt(interp, alv[1], &address)!=TCL_OK)
                goto fehler;
            addr=new C_io_addr_raw(io_out, address);
            }
            break;
        case Addr_Modul:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... modul name",
                    -1));
                goto fehler;
            }
            addr=new C_io_addr_modul(io_out, alv[1]);
            break;
        case Addr_Driver_mapped:
        case Addr_Driver_mixed:
        case Addr_Driver_syscall:
            Tcl_SetObjResult(interp,
                Tcl_NewStringObj("Addresstype not supported", -1));
            goto fehler;
            break;
        case Addr_Socket: {
            int port;
            if (alc==2) {
                if (Tcl_GetInt(interp, alv[1], &port)!=TCL_OK)
                    goto fehler;
                addr=new C_io_addr_socket(io_out, port);
            } else if (alc==3) {
                if (Tcl_GetInt(interp, alv[2], &port)!=TCL_OK)
                goto fehler;
                addr=new C_io_addr_socket(io_out, alv[1], port);
            } else {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... "
                        "socket [host] port",
                        -1));
                goto fehler;
            }
            }
            break;
        case Addr_V6Socket: {
            if (alc==2) {
                addr=new C_io_addr_v6socket(io_out, ip_passive, alv[1]);
            } else if (alc==3) {
                addr=new C_io_addr_v6socket(io_out, ip_default,
                        alv[1], alv[2]);
            } else {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... "
                        "v6socket [host] port",
                        -1));
                goto fehler;
            }
            }
            break;
        case Addr_Autosocket: {
            int port;
            if (alc==2) {
                if (Tcl_GetInt(interp, alv[1], &port)!=TCL_OK)
                    goto fehler;
                addr=new C_io_addr_autosocket(io_out, port);
            } else {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... "
                        "autosocket port",
                        -1));
                goto fehler;
            }
            }
            break;
        case Addr_LocalSocket:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... "
                        "localsocket name",
                        -1));
                goto fehler;
            }
            addr=new C_io_addr_localsocket(io_out, alv[1]);
            break;
        case Addr_File:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... file name",
                    -1));
                goto fehler;
            }
            addr=new C_io_addr_file(io_out, alv[1]);
            break;
        case Addr_AsynchFile:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... file name",
                    -1));
                goto fehler;
            }
            addr=new C_io_addr_asynchfile(io_out, alv[1]);
            break;
        case Addr_Tape:
            if (alc!=2) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... tape name",
                    -1));
                goto fehler;
            }
            addr=new C_io_addr_tape(io_out, alv[1]);
            break;
        case Addr_Null:
            if (alc!=1) {
                Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("wrong # args; must be ... null",
                    -1));
                goto fehler;
            }
            addr=new C_io_addr_null(io_out);
            break;
        }
        io_addr=new C_data_io(typ, addr);
        typ=0; addr=0;
        io_addr->forcelists(uselists);
        if (clv) {
            int* clvi=new int[clc];
            int i;
            for (i=0; i<clc; i++) {
                if (Tcl_GetInt(interp, clv[i], clvi+i)!=TCL_OK)
                    goto fehler;
            }
            io_addr->set_args(clvi, clc);
        }
        DownloadDataout(idx, *io_addr);
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        goto fehler;
    }
    res=TCL_OK;
    goto raus;
fehler:
    res=TCL_ERROR;
raus:
    if (uselists) {
        Tcl_Free((char*)blv);
        Tcl_Free((char*)alv);
        Tcl_Free((char*)clv);
    }
    delete[] blvi;
    delete typ;
    delete addr;
    delete io_addr;
    return res;
}
/*****************************************************************************/

int E_ved::e_dataout_delete(int argc, const char* argv[])
{
// <VED> dataout delete idx
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=4)
  {
  Tcl_SetObjResult(interp,
        Tcl_NewStringObj("wrong # args; must be dataout delete idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  DeleteDataout(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss<<last_xid_;
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

int E_ved::e_dataout_status(int argc, const char* argv[])
{
// <VED> dataout status idx [device]
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_dostatus)!=TCL_OK)
      return TCL_ERROR;
  }
else
  {
  if (confback.finit(interp, this, argc, argv, &E_ved::f_dostatus)!=TCL_OK)
      return TCL_ERROR;
  }

if ((argc<4) || (argc>5))
  {
  Tcl_SetObjResult(interp,
        Tcl_NewStringObj("wrong # args; must be dataout status idx [arg]", -1));
  return TCL_ERROR;
  }
int idx, arg;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
if (argc>4)
  {
  if (Tcl_GetInt(interp, argv[4], &arg)!=TCL_OK) return TCL_ERROR;
  }
else
  arg=0;
try
  {
  if (confmode_==synchron)
    {
    C_confirmation* conf=GetDataoutStatus(idx, arg);
    ostringstream ss;
    //f_dostatus(ss, conf, 0, 0, 0);
    confback.callformer(this, ss, conf);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    GetDataoutStatus(idx, arg);
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_dataout_wind(int argc, const char* argv[])
{
// <VED> dataout wind idx offset
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=5)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be dataout wind idx offset",
      -1));
  return TCL_ERROR;
  }
int idx, offset;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
if (strncmp(argv[4], "end", strlen(argv[4]))==0)
  offset=2000;
else if (strncmp(argv[4], "begin", strlen(argv[4]))==0)
  offset=-2000;
else
  {if (Tcl_GetInt(interp, argv[4], &offset)!=TCL_OK) return TCL_ERROR;}
try
  {
  WindDataout(idx, offset);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss<<last_xid_;
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
int E_ved::writedataout(E_ved::confbackbox& confback, int idx,
    int default_header, C_outbuf& ob)
{
int res=TCL_OK;
try
  {
  WriteDataout(idx, default_header, ob);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_ ;
    Tcl_SetResult_Stream(interp, ss);
    }
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  res=TCL_ERROR;
  }
return res;
}
/*****************************************************************************/
int E_ved::e_dataout_write(int argc, const char* argv[])
{
// <VED> dataout write idx stringlist
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=5)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be dataout write idx stringlist",
      -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
C_outbuf ob;

int blc;
const char **blv;
if (Tcl_SplitList(interp, argv[4], &blc, &blv)!=TCL_OK) return TCL_ERROR;
ob<<blc;
for (int i=0; i<blc; i++) ob<<blv[i];
Tcl_Free((char*)blv);
return writedataout(confback, idx, 2 /*clusterty_text*/, ob);
}
/*****************************************************************************/
int E_ved::e_dataout_writefile(int argc, const char* argv[])
{
// <VED> dataout write idx filename [to]
// confbackbox confback;
// if (confmode_==asynchron)
//   {
//   if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
//       return TCL_ERROR;
//   }
    if (confmode_==asynchron) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("dataout write file can not be done asynchronously",
            -1));
        return TCL_ERROR;
    }
    if ((argc<5) && (argc>6)) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be dataout write idx filename "
            "[timeout/sec]",
            -1));
        return TCL_ERROR;
    }

    int idx, to;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
    if (argc>5) {
        if (Tcl_GetInt(interp, argv[5], &to)!=TCL_OK) return TCL_ERROR;
    } else {
        to=120;
    }

    const char* filename=argv[4];
    int f;
    f=open(filename, O_RDONLY, 0);
    if (f<0) {
        ostringstream ss;
        ss << "cannot open "<<filename<<": "<<strerror(errno);
        Tcl_SetResult_Stream(interp, ss);
        return TCL_ERROR;
    }
    struct stat buffer;
    if (fstat(f, &buffer)<0) {
        ostringstream ss;
        ss << "cannot stat "<<filename<<": "<<strerror(errno);
        Tcl_SetResult_Stream(interp, ss);
        close(f);
        return TCL_ERROR;
    }
    if (!S_ISREG(buffer.st_mode)) {
        ostringstream ss;
        ss <<filename<<" is not a regular file";
        Tcl_SetResult_Stream(interp, ss);
        close(f);
        return TCL_ERROR;
    }

    char *map, *mapp;
    const int maxblock(64000);
    if (buffer.st_size) {
        map=(char*)mmap(0, buffer.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE, f, 0);
        if (map==MAP_FAILED) {
            ostringstream ss;
            ss<<"cannot map "<<filename<<": "<<strerror(errno);
            Tcl_SetResult_Stream(interp, ss);
            close(f);
            return TCL_ERROR;
        }
    } else {
        map=0;
    }
    int num_recs=(buffer.st_size+maxblock-1)/maxblock;
    if (!num_recs) num_recs=1;
    close(f);
    mapp=map;

    struct timeval start;
    gettimeofday(&start, 0);
    int res=TCL_OK;
    for (int rec=0; (rec<num_recs) && (res==TCL_OK); rec++) {
        //cerr<<"writing part "<<rec<<endl;
        C_outbuf ob;
        ob<<4 /* clusterty_file */;
        ob<<1 /* ClOPTFL_TIME */;
        ob<<((rec+1<num_recs)?3:1) /* flags*/;
        ob<<rec;
        ob<<filename;
        ob<<(int)buffer.st_ctime;
        ob<<(int)buffer.st_mtime;
        ob<<(int)buffer.st_mode;
        ob<<(int)buffer.st_size;
        int len=(rec+1<num_recs)?maxblock:buffer.st_size%maxblock;
        ob.putchararr(mapp, len);
        int weiter=0;
        do {
            try {
                WriteDataout(idx, 0, ob);
                gettimeofday(&start, 0); // successfull, reset time
                weiter=0;
            } catch(C_ved_error* e) {
                if ((e->errtype()==C_ved_error::req_err) &&
                        (e->error().req_err==Err_NoMem)) {
                    cerr<<"e_dataout_writefile: Err_NoMem"<<endl;
                    cerr<<"  "<<filename<<" ("<<(void*)mapp<<")"<<endl;
                    struct timeval now;
                    gettimeofday(&now, 0);
                    int tdiff=now.tv_sec-start.tv_sec;
                    if (now.tv_usec<start.tv_usec) tdiff--;
                    weiter=tdiff<to;
                    if (weiter) {
                        sleep(1);
                    } else {
                        Tcl_SetResult_Err(interp, e);
                        res=TCL_ERROR;
                    }
                } else {
                    Tcl_SetResult_Err(interp, e);
                    res=TCL_ERROR;
                }
                delete e;
            } catch(C_error* e) {
                Tcl_SetResult_Err(interp, e);
                delete e;
                res=TCL_ERROR;
            }
        } while (weiter);
        mapp+=len;
    }
    if (buffer.st_size) munmap(map, buffer.st_size);
    return res;
}
/*****************************************************************************/

int E_ved::e_dataout_enable(int argc, const char* argv[])
{
// <VED> dataout enable idx 0|1
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }
if (argc!=5)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be dataout enable idx 0|1",
      -1));
  return TCL_ERROR;
  }
int idx, ena;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
if (Tcl_GetInt(interp, argv[4], &ena)!=TCL_OK) return TCL_ERROR;
try
  {
  EnableDataout(idx, ena);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss<<last_xid_;
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
/*****************************************************************************/

int E_ved::e_is(int argc, const char* argv[])
{
// <VED> is list|exists|create|open|delete|id
//

int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
  res=TCL_ERROR;
  }
else
  {
  const char* names[]={
    "list",
    "exists",
    "create",
    "open",
    "delete",
    "id",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_is_list(argc, argv); break;
    case 1: res=e_is_exists(argc, argv); break;
    case 2: res=e_is_create(argc, argv); break;
    case 3: res=e_is_open(argc, argv); break;
    case 4: res=e_is_delete(argc, argv); break;
    case 5: res=e_is_id(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/

int E_ved::e_is_list(int argc, const char* argv[])
//is list
{
if (argc!=3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be is list", -1));
  return TCL_ERROR;
  }
try
  {
  C_confirmation* conf;
  conf=GetNamelist(Object_is);
  C_inbuf ib(conf->buffer(), conf->size());
  delete conf;
  ib++;
  int num=ib.getint();
  for (int i=0; i<num; i++)
    {
    ostringstream ss;
    ss << ib.getint();
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
int E_ved::e_is_exists(int argc, const char* argv[])
//is exists idx
{
    if (argc!=4) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be is exists idx", -1));
        return TCL_ERROR;
    }
    int idx;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK)
        return TCL_ERROR;
    try {
        C_confirmation* conf;
        conf=GetNamelist(Object_is);
        C_inbuf ib(conf->buffer(), conf->size());
        delete conf;
        ib++;
        int num=ib.getint();
        int found=0;
        for (int i=0; !found && (i<num); i++) {
            if ((int)ib.getint()==idx)
                found=1;
        }
        Tcl_SetObjResult(interp, Tcl_NewIntObj(found));
    } catch(C_error* error) {
        Tcl_SetResult_Err(interp, error);
        delete error;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*****************************************************************************/

int E_ved::e_is_open(int argc, const char* argv[])
//is open idx [id]
{
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_createis)!=TCL_OK)
      return TCL_ERROR;
  }
if ((argc!=4) && (argc!=5))
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be is open idx [id]", -1));
  return TCL_ERROR;
  }
int idx, id;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
if ((argc==5) && Tcl_GetInt(interp, argv[4], &id)!=TCL_OK) return TCL_ERROR;
if (confmode_==synchron)
  {
  E_is* is;
  try
    {
    if (argc==4) // do not create is
      is=new E_is(interp, this, idx, C_instr_system::open);
    else
      is=new E_is(interp, this, idx, C_instr_system::create, id);
    }
  catch(C_error* e)
    {
    Tcl_SetResult_Err(interp, e);
    delete e;
    return TCL_ERROR;
    }
  is->create_tcl_proc();
  Tcl_SetObjResult(interp, Tcl_NewStringObj((char*)is->origtclprocname(), -1));
  return TCL_OK;
  }
else
  {
  try
    {
    if (argc==4) // do not create is
      {
      GetNamelist(Object_is);
      if (confback.isimplicit())
        {
        ostringstream ss;
        ss << idx;
        string s=ss.str();
        confback.formargs(1, s.c_str());
        confback.formargs(0, "open");
        }
      confback.xid(last_xid_);
      installconfback(confback);
      ostringstream ss;
      ss << last_xid_;
      Tcl_SetResult_Stream(interp, ss);
      return TCL_OK;
      }
    else
      {
      Tcl_SetObjResult(interp, Tcl_NewStringObj("this operation is not possible in asynchronous "
          "mode; use 'is create'",
          -1));
      return TCL_ERROR;
      }
    }
  catch(C_error* e)
    {
    Tcl_SetResult_Err(interp, e);
    delete e;
    return TCL_ERROR;
    }
  }
}

/*****************************************************************************/
int E_ved::e_is_create(int argc, const char* argv[])
//is create idx id
{
    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_createis)!=TCL_OK)
            return TCL_ERROR;
    }
    if (argc!=5) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj("wrong # args; must be is create idx id",
            -1));
        return TCL_ERROR;
    }
    int idx, id;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
    if (Tcl_GetInt(interp, argv[4], &id)!=TCL_OK) return TCL_ERROR;
    if (confmode_==synchron) {
        E_is* is;
        try {
            is=new E_is(interp, this, idx, C_instr_system::create, id);
        } catch(C_error* e) {
            Tcl_SetResult_Err(interp, e);
            delete e;
            return TCL_ERROR;
        }
        is->create_tcl_proc();
        Tcl_SetObjResult(interp, Tcl_NewStringObj(is->origtclprocname(), -1));
    } else {
        try {
            create_is(idx, id);
            if (confback.isimplicit()) {
                ostringstream ss;
                ss << idx;
                string s=ss.str();
                confback.formargs(1, s.c_str());
                confback.formargs(0, "create");
            }
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        } catch(C_error* e) {
            Tcl_SetResult_Err(interp, e);
            delete e;
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}
/*****************************************************************************/

int E_ved::e_is_delete(int argc, const char* argv[])
{
//is delete idx
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be is delete idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  DeleteIS(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_is_id(int argc, const char* argv[])
{
//is id idx
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_integer)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong # args; must be is id idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    C_isstatus* status=ISStatus(idx);
    ostringstream ss;
    ss << status->id();
    delete status;
    Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    ISStatus(idx);
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_openis(int argc, const char* argv[])
//openis
{
if (argc!=2)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("no args expected", -1));
  return TCL_ERROR;
  }
Tcl_ResetResult(interp);
// is_entry(0) ist is_0; kann nicht geoeffnet sein
for (int i=1; i<is_num(); i++)
  {
  Tcl_AppendElement(interp, (char*)((E_is*)is_entry(i))->tclprocname());
  }
return TCL_OK;
}

/*****************************************************************************/

int E_ved::e_caplist(int argc, const char* argv[])
{
if (argc>3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be caplist [type]", -1));
  return TCL_ERROR;
  }
Capabtyp typ=Capab_listproc;
if (argc==3)
  {
  const char* names[]={"proc", "trigg", 0};
  switch (findstring(interp, argv[2], names))
    {
    case 0:
      typ=Capab_listproc;
      break;
    case 1:
      typ=Capab_trigproc;
      break;
    default:
      return TCL_ERROR;
    }
  }
try
  {
  string name;
  int num=numprocs(typ);
  for(int id=0; id<num; id++)
    {
    ostringstream ss;
    ss << id << ' ' << procname(id, typ);
    Tcl_AppendElement_Stream(interp, ss);
    }
  }
catch(C_ptr_error* e)
  {
  delete e;
  Tcl_SetObjResult(interp, Tcl_NewStringObj("server has no such capabilitylist", -1));
  Tcl_SetErrorCode(interp, "EMS", "other", (char*)0);
  return TCL_ERROR;
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

int E_ved::e_version_separator(int argc, const char* argv[])
{
if (argc>3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be version_separator [separator]",
      -1));
  return TCL_ERROR;
  }
int res;
if (argc==2)
  res=version_separator(Capab_listproc);
else
  {
  version_separator(argv[2][0], Capab_trigproc);
  res=version_separator(argv[2][0], Capab_listproc);
  }
char s[2];
s[0]=res;
s[1]=0;
Tcl_SetResult(interp, s, TCL_VOLATILE);
return TCL_OK;
}

/*****************************************************************************/
int E_ved::e_command(int argc, const char* argv[])
{
// ved_<vedname> command {name1 {arg1 arg2 ...} name2 {arg1 arg2 ...} ...}
//
    {
        const char* var=Tcl_GetVar(interp, "ems_debug", TCL_GLOBAL_ONLY);
        int ivar;

        if (var && (Tcl_GetInt(interp, var, &ivar)==TCL_OK) && (ivar>0)) {
            cerr<<"command for VED "<<argv[0]<<": "<<argc<<" arguments"<<endl;
            for (int i=0; i<argc; i++)
                cerr<<"["<<i<<"]: >"<<argv[i]<<"<"<<endl;
        }
    }

    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_command)!=TCL_OK)
        return TCL_ERROR;
    } else {
        if (confback.finit(interp, this, argc, argv, &E_ved::f_command)!=TCL_OK)
            return TCL_ERROR;
    }

    if (is0_==0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("VED has no local procedures", -1));
        return TCL_ERROR;
    }

    if (argc!=3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be\n"
                "  ved_<vedname> command {name1 {arg1 arg2 ...} name2 {arg1 arg2 ...} ...}",
                -1));
        return TCL_ERROR;
    }

    int plc;
    const char** plv;
    if (Tcl_SplitList(interp, argv[2], &plc, &plv)!=TCL_OK)
        return TCL_ERROR;
    if (plc%2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("arguments missing", -1));
        Tcl_Free((char*)plv);
        return TCL_ERROR;
    }

    int res=TCL_OK;
    exec_mode oldmode=is0_->execution_mode(delayed);
    is0_->clear_command();
    for (int n=0; (res==TCL_OK) && (n<plc); n+=2) {
        int pc;
        const char** pv;
        if (Tcl_SplitList(interp, plv[n+1], &pc, &pv)==TCL_OK) {
            try {
                is0_->command(plv[n]);
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
                            is0_->add_param(s+1);
                            *se='\'';
                        }
                    } else {
                        long val;
                        if (xTclGetLong(interp, (char*)pv[i], &val)!=TCL_OK)
                            res=TCL_ERROR;
                        else
                            is0_->add_param((int)val);
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
        is0_->clear_command();
        is0_->execution_mode(oldmode);
        return res;
    }

    try {
        if (confmode_==synchron) {
            C_confirmation* conf=is0_->execute();
            ostringstream ss;
            //f_command(ss, conf, 0, 0, 0);
            confback.callformer(this, ss, conf);
            Tcl_SetResult_Stream(interp, ss);
            delete conf;
        } else {
            is0_->execute();
            confback.xid(is0_->last_xid());
            installconfback(confback);
            ostringstream ss;
            ss << is0_->last_xid();
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        res=TCL_ERROR;
    }
    is0_->execution_mode(oldmode);
    return res;
}
/*****************************************************************************/
int
E_ved::e_command1(int argc, const char* argv[])
{
// ved_<vedname> command1 name [arg1 ...]
//
    {
        const char* var=Tcl_GetVar(interp, "ems_debug", TCL_GLOBAL_ONLY);
        int ivar;
        if (var && (Tcl_GetInt(interp, var, &ivar)==TCL_OK) && (ivar>0)) {
            cerr<<"command1 for VED "<<argv[0]<<": "<<argc<<" arguments"<<endl;
            for (int i=0; i<argc; i++)
                cerr<<"["<<i<<"]: >"<<argv[i]<<"<"<<endl;
        }
    }
    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_command)!=TCL_OK)
            return TCL_ERROR;
    } else {
        if (confback.finit(interp, this, argc, argv, &E_ved::f_command)!=TCL_OK)
            return TCL_ERROR;
    }
    if (is0_==0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("VED has no local procedures", -1));
        return TCL_ERROR;
    }
    if (argc<3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be\n"
            "  ved_<vedname> command1 name [arg1 ...]", -1));
        return TCL_ERROR;
    }
    int res=TCL_OK;
    exec_mode oldmode=is0_->execution_mode(delayed);
    is0_->clear_command();
    try {
        is0_->command(argv[2]);
        for (int i=3; (res==TCL_OK) && (i<argc); i++) {
            const char* p=index(argv[i], '.');
            char c=argv[i][0];
            if (c=='\'') {
                char* s=(char*)argv[i];
                char* se=s+strlen(s)-1;
                if (*se!='\'') {
                    ostringstream ss;
                    ss << "Unmatched ' in " << s;
                    Tcl_SetResult_Stream(interp, ss);
                    res=TCL_ERROR;
                } else {
                    *se=0;
                    is0_->add_param(s+1);
                    *se='\'';
                }
            } else if (p) {
#if 1
                Tcl_SetObjResult(interp, Tcl_NewStringObj("float argument not supported",
                    -1));
                res=TCL_ERROR;
#else
                double val;
                char* cp;
                if ((c=='d') || (c=='f'))
                    cp=(char*)argv[i]+1;
                else
                    cp=(char*)argv[i];
                if (Tcl_GetDouble(interp, cp, &val)!=TCL_OK) {
                    res=TCL_ERROR;
                } else {
                    if (c=='f')
                        is0_->add_param((float)val);
                    else
                        is0_->add_param((double)val);
                }
#endif
            } else {
                long val;
                if (xTclGetLong(interp, (char*)argv[i], &val)!=TCL_OK)
                    res=TCL_ERROR;
                else
                    is0_->add_param((int)val);
            }
        }
        if (res==TCL_OK) {
            if (confmode_==synchron) {
                C_confirmation* conf=is0_->execute();
                ostringstream ss;
                //f_command(ss, conf, 0, 0, 0);
                confback.callformer(this, ss, conf);
                Tcl_SetResult_Stream(interp, ss);
                delete conf;
            } else {
                is0_->execute();
                confback.xid(is0_->last_xid());
                installconfback(confback);
                ostringstream ss;
                ss << is0_->last_xid();
                Tcl_SetResult_Stream(interp, ss);
            }
        }
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        res=TCL_ERROR;
    }
    is0_->execution_mode(oldmode);
    return res;
}
/*****************************************************************************/
int
E_ved::e_modullist(int argc, const char* argv[])
// <VED> modullist create|[upload]|delete
{
    int res;
    if (argc<3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be modullist "
                "create|upload|delete", -1));
        return TCL_ERROR;
    }

    const char* names[]={
        "create",
        "upload",
        "delete",
        0
    };
    switch (findstring(interp, argv[2], names)) {
    case 0:
        res=e_modlist_create(argc, argv);
        break;
    case 1:
        res=e_modlist_upload(argc, argv);
        break;
    case 2:
        res=e_modlist_delete(argc, argv);
        break;
    default:
        res=TCL_ERROR;
    }
    return res;
}
/*****************************************************************************/
#if 0
static int
parse_ml_unspec(Tcl_Interp* interp, Modulclass modulclass,
    int mc, const char* mv[], C_modullist::ml_entry* entry)
{
    int addr, typ;
    entry->modulclass=modul_unspec;
    if (mc!=2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("parse_ml_unspec: 2 arguments expected",
                -1));
        return TCL_ERROR;
    }
    if ((Tcl_GetInt(interp, mv[0], &addr)!=TCL_OK)
     || (Tcl_GetInt(interp, mv[1], &typ)!=TCL_OK)) {
        return TCL_ERROR;
    }
    entry->modultype=typ;
    entry->address.unspec.addr=addr;
    return TCL_OK;
}
#endif
/*****************************************************************************/
static int
parse_ml_generic(Tcl_Interp* interp, Modulclass modulclass,
    int mc, const char* mv[], C_modullist::ml_entry* entry)
{
    entry->modulclass=modul_generic;
    Tcl_SetObjResult(interp, Tcl_NewStringObj("modul_generic not yet implemented", -1));
    return TCL_ERROR;
}
/*****************************************************************************/
static int
parse_ml_adr2(Tcl_Interp* interp, Modulclass modulclass,
    int mc, const char* mv[], C_modullist::ml_entry* entry)
{
    int addr, typ, crate;
    entry->modulclass=modulclass;
    if (mc!=3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("parse_ml_adr2: 3 arguments expected",
                -1));
        return TCL_ERROR;
    }
    if ((Tcl_GetInt(interp, mv[0], &typ)!=TCL_OK)
     || (Tcl_GetInt(interp, mv[1], &addr)!=TCL_OK)
     || (Tcl_GetInt(interp, mv[2], &crate)!=TCL_OK)) {
        return TCL_ERROR;
    }
    entry->modultype=typ;
    entry->address.adr2.addr=addr;
    entry->address.adr2.crate=crate;
    return TCL_OK;
}
/*****************************************************************************/
static int
parse_ml_ip(Tcl_Interp* interp,
    int mc, const char* mv[], C_modullist::ml_entry* entry)
{
    int typ;

    /* we have to set these members before any error can occur;
       otherwise the destructor will free invalid pointers */
    entry->modulclass=modul_ip;
    entry->address.ip.address=0;
    entry->address.ip.protocol=0;
    entry->address.ip.rport=0;
    entry->address.ip.lport=0;

    if (mc<2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("parse_ml_ip: "
            "expected arguments: type address [ protocol [ rport [ lport ]]]",
            -1));
        return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, mv[0], &typ)!=TCL_OK)
        return TCL_ERROR;

    entry->modultype=typ;
    entry->address.ip.address=strdup(mv[1]);
    if (mc>2)
        entry->address.ip.protocol=strdup(mv[2]);
    if (mc>3)
        entry->address.ip.rport=strdup(mv[3]);
    if (mc>4)
        entry->address.ip.lport=strdup(mv[4]);

    return TCL_OK;
}
/*****************************************************************************/
static int
parse_mlentry(Tcl_Interp* interp, const char* mlv, C_modullist* mlist)
{
    const char **mv;
    int mc, res=TCL_OK;
    C_modullist::ml_entry entry;

    if (Tcl_SplitList(interp, mlv, &mc, &mv)!=TCL_OK)
        return TCL_ERROR;
    Modulclass modulclass=(Modulclass)findstring(interp, mv[0],
            Modulclass_names);

    switch (modulclass) {
    case modul_none:
        entry.modulclass=modul_none;
        break;
    case modul_unspec_:
        throw new C_program_error(
            "parse_mlentry: modul_unspec no longer supported");
#if 0
        res=parse_ml_unspec(interp, modulclass, mc-1, mv+1, &entry);
        if (res)
            goto error;
#endif
        break;
    case modul_generic:
        res=parse_ml_generic(interp, modulclass, mc-1, mv+1, &entry);
        if (res)
            goto error;
        break;
    case modul_camac:
    case modul_fastbus:
    case modul_vme:
    case modul_lvd:
    case modul_can:
    case modul_caenet:
    case modul_sync:
    case modul_perf:
    case modul_pcihl:
        res=parse_ml_adr2(interp, modulclass, mc-1, mv+1, &entry);
        if (res)
            goto error;
        break;
    case modul_ip:
        res=parse_ml_ip(interp, mc-1, mv+1, &entry);
        if (res)
            goto error;
        break;
    case modul_invalid:
        entry.modulclass=modul_invalid;
        break;
    default:
        {
            ostringstream ss;
            ss<<"modulclass "<<mv[0]<<" not known or not supported";
            Tcl_SetResult_Stream(interp, ss);
        }
        res=TCL_ERROR;
        goto error;
    }
    mlist->add(entry);

error:
    Tcl_Free((char*)mv);
    return res;
}
/*****************************************************************************/
static int
parse_modullist(Tcl_Interp* interp, int argc, const char* argv[],
    C_modullist* mlist)
{
    // sanity check
    if (argc<1) {
        // causes memory leak in E_ved::e_modlist_create (mlv)
        // but this error is fatal anyway
        throw new C_program_error("emstcl_vedcomm.cc:parse_modullist: argc<1");
    }

    // check whether argv[0] is a list or a single value
    const char **mv;
    int mc;
    if (Tcl_SplitList(interp, argv[0], &mc, &mv)==TCL_OK)
        Tcl_Free((char*)mv);
    else
        mc=1;

    if (mc>=2) { // it is a list
        int res, i;
        for (i=0; i<argc; i++) {
            res=parse_mlentry(interp, argv[i], mlist);
            if (res!=TCL_OK)
                return res;
        }
    } else { // it is a single value
        //throw new C_program_error(
        //    "parse_modullist: old version no longer supported");
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong args: we need a list",
                -1));
        return TCL_ERROR;

#if 0
        const char* arg2[2];
        int res, i;
        if (argc&1) { // we need pairs of values
            Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be pairs of {type, address}",
                -1));
            return TCL_ERROR;
        }
        for (i=0; i<argc; i+=2) {
            C_modullist::ml_entry entry;
            arg2[0]=argv[i];
            arg2[1]=argv[i+1];
            res=parse_ml_unspec(interp, modul_unspec, 2, arg2, &entry);
            if (res!=TCL_OK)
                return res;
            mlist->add(entry);
        }
#endif
    }
    
    return TCL_OK;
}
/*****************************************************************************/
int E_ved::e_modlist_create(int argc, const char* argv[])
{
// <VED> modullist create <arguments>
//
// types of arguments:
// no arguments                                type 0a
// {}                                          type 0b
// <type> <addr> <type> <addr> ....            type 1a
// {<type> <addr> <type> <addr> ....}          type 1b
// {<type> <addr>} {<type> <addr>} ....        type 2a
// {{<type> <addr>} {<type> <addr>} ....}      type 2b
// {<modulclass> <args for modulclass>} ....   type 3a
// {{<modulclass> <args for modulclass>} ....} type 3b
//
// modulclass and args can be:
// unspec <slot> <type>          !!! reverse order of type and slot
// generic (not fixed yet)
// camac <type> <slot> <crate>
// fastbus <type> <slot> <crate>
// vme <type> <addr> <crate>
// lvd <type> <addr> <crate>
// perf <type> <addr> <crate>
// can <type> <addr> <crate>

    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    C_modullist mlist(0);

    // argc is at least 3 because "<VED> modullist create" is already checked
    switch (argc) {
    case 3: // no argument given --> empty list (type 0a)
        break;
    case 4: { // must be a list of modules (0b 1b 2b 3b)
            const char **mlv;
            int mlc, res;
            if (Tcl_SplitList(interp, argv[3], &mlc, &mlv)!=TCL_OK)
                return TCL_ERROR;
            if (mlc>0) { // list not empty
                res=parse_modullist(interp, mlc, mlv, &mlist);
                Tcl_Free((char*)mlv);
                if (res!=TCL_OK)
                    return res;
            } // else empty list (0b)
        }
        break;
    default: { // (1a 2a 3a)
            int res;
            res=parse_modullist(interp, argc-3, argv+3, &mlist);
            if (res!=TCL_OK)
                return res;
        }
    }

#if 0
    int version=0;
    for (int i=0; i<mlist.size(); i++) {
        if (mlist.modulclass(i)!=modul_unspec)
            version=1;
    }
#else
    /* modul_unspec does not exist any more, therefore its usage is a
       fatal error */
    for (int i=0; i<mlist.size(); i++) {
        if (mlist.modulclass(i)==modul_unspec_)
            throw new C_program_error(
                "E_ved::e_modlist_create: modul_unspec not supported");
    }
#endif

    int res;
    try {
        DownloadModullist(mlist, 1 /*version*/);
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            installconfback(confback);
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
    return res;
}
/*****************************************************************************/
int E_ved::e_modlist_upload(int argc, const char* argv[])
{
// <VED> modullist upload [0|1]
//
    confbackbox confback;
    int version;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_modullist)
                !=TCL_OK)
            return TCL_ERROR;
    }

    if (argc<3 || argc>4) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be modullist upload [version]",
            -1));
        return TCL_ERROR;
    }

    if (argc==4) {
        if (Tcl_GetInt(interp, argv[3], &version)!=TCL_OK) return TCL_ERROR;
    } else {
        version=0;
    }

    try {
        if (confmode_==synchron) {
            C_modullist* mlist=UploadModullist(version);
            ostringstream ss;
            ff_modullist(ss, mlist, version);
            delete mlist;
            Tcl_SetResult_Stream(interp, ss);
        } else {
            UploadModullist(version);
            if (confback.isimplicit())
                confback.formargs(0, argv[3]);
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*****************************************************************************/

int E_ved::e_modlist_delete(int argc, const char* argv[])
{
// <VED> modullist delete
//

if (argc!=3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be modullist delete",
      -1));
  return TCL_ERROR;
  }

try
  {
  DeleteModullist();
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

int E_ved::e_lam(int argc, const char* argv[])
{
// <VED> lam create|delete|upload|start|stop|resume|reset|status idx 

int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam create|delete|upload|start"
      "|stop|resume|reset|status idx",
      -1));
  return TCL_ERROR;
  }
else
  {
  const char* names[]={
    "create",
    "delete",
    "status",
    "start",
    "stop",
    "resume",
    "reset",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_lam_create(argc, argv); break;
    case 1: res=e_lam_delete(argc, argv); break;
    case 2: res=e_lam_status(argc, argv); break;
    case 3: res=e_lam_start(argc, argv); break;
    case 4: res=e_lam_stop(argc, argv); break;
    case 5: res=e_lam_resume(argc, argv); break;
    case 6: res=e_lam_reset(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/
#if 1
int E_ved::e_lam_create(int argc, const char* argv[])
{
// <VED> lam create idx id is trigproc trigprocargs ...
//
    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc<7) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam create idx id is "
                "trigproc args", -1));
        return TCL_ERROR;
    }

    int idx, id, is;
    const char *trigproc;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
    if (Tcl_GetInt(interp, argv[4], &id)!=TCL_OK) return TCL_ERROR;
    if (Tcl_GetInt(interp, argv[5], &is)!=TCL_OK) return TCL_ERROR;
    trigproc=argv[6];

    int *args=0, argnum=argc-7;
    if (argnum) {
        args=new int[argnum];
        for (int i=7; i<argc; i++) {
            if (Tcl_GetInt(interp, argv[i], args+i-7)!=TCL_OK) {
                delete[] args;
                return TCL_ERROR;
            }
        }
    }

    int res=TCL_OK;
    try {
        CreateLam(idx, id, is, trigproc, args, argnum);
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch(C_error* e) {
         Tcl_SetResult_Err(interp, e);
        delete e;
        res=TCL_ERROR;
    }
    delete[] args;
    return res;
}
#else
int E_ved::e_lam_create(int argc, const char* argv[])
{
// <VED> lam create idx id is {arg}
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }
int res;

if (argc<6)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam create idx id is {args}",
      -1));
  return TCL_ERROR;
  }
int idx, id, is;
int argnum, *args;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
if (Tcl_GetInt(interp, argv[4], &id)!=TCL_OK) return TCL_ERROR;
if (Tcl_GetInt(interp, argv[5], &is)!=TCL_OK) return TCL_ERROR;
args=0; argnum=argc-6;
if (argnum)
  {
  args=new int[argnum];
  for (int i=6; i<argc; i++)
    {
    if (Tcl_GetInt(interp, argv[i], args+i-6)!=TCL_OK)
      {
      delete[] args;
      return TCL_ERROR;
      }
    }
  }
try
  {
  CreateLam(idx, id, is, args, argnum);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  res=TCL_OK;
  }
catch(C_error* e)
  {
  Tcl_SetResult_Err(interp, e);
  delete e;
  res=TCL_ERROR;
  }
delete[] args;
return res;
}
#endif
/*****************************************************************************/

int E_ved::e_lam_delete(int argc, const char* argv[])
{
// <VED> lam delete idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam delete idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  DeleteLam(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_lam_status(int argc, const char* argv[])
{
// <VED> lam status idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_lamstatus)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam status idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  if (confmode_==synchron)
    {
    C_confirmation* conf=UploadLam(idx);
    ostringstream ss;
    f_lamstatus(ss, conf, 0, 0, 0);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
    }
  else
    {
    UploadLam(idx);
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_lam_start(int argc, const char* argv[])
{
// <VED> lam start idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam start idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  StartLam(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_lam_stop(int argc, const char* argv[])
{
// <VED> lam stop idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam stop idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  StopLam(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_lam_resume(int argc, const char* argv[])
{
// <VED> lam resume idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam resume idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  ResumeLam(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_lam_reset(int argc, const char* argv[])
{
// <VED> lam reset idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lam reset idx", -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  ResetLam(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_lamproclist(int argc, const char* argv[])
{
// <VED> lamproclist create|delete|upload 

int res;
if (argc<3)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lamproclist "
      "create|delete|upload",
      -1));
  return TCL_ERROR;
  }
else
  {
  const char* names[]={
    "create",
    "delete",
    "upload",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_lamproclist_create(argc, argv); break;
    case 1: res=e_lamproclist_delete(argc, argv); break;
    case 2: res=e_lamproclist_upload(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/
int E_ved::e_lamproclist_setup(int argc, const char* argv[])
{
// name1 {arg1 ...} name2 {arg1 ...} ...
// called from  e_lamproclist_create

    int res=TCL_OK;
    clear_lamlist();

    if (argc%2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args in E_ved::e_lamproclist_setup "
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
            lam_add_proc(proc);
        } catch (C_error* e) {
            Tcl_SetResult_Err(interp, e);
            delete e;
            res=TCL_ERROR;
        }
        for (int j=0; (res==TCL_OK) && (j<mlc); j++) {
            long arg;
            if (xTclGetLong(interp, (char*)mlv[j], &arg)!=TCL_OK) {
                res=TCL_ERROR;
                break;
            }
            try {
                lam_add_param((int)arg);
            }catch (C_error* e) {
                Tcl_SetResult_Err(interp, e);
                delete e;
                res=TCL_ERROR;
            }
        }
        Tcl_Free((char*)mlv);
    }
    if (res!=TCL_OK)
        clear_lamlist();

    return res;
}
/*****************************************************************************/
int E_ved::e_lamproclist_create(int argc, const char* argv[])
{
// <VED> lamproclist create idx unsol {name1 {arg1 arg2 ...} ...}
// <VED> lamproclist create idx unsol name1 {arg1 ...} name2 {arg1 ...} ...
//
    confbackbox confback;
    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if ((argc<5) || ((argc>6) && (argc%2==0))) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be "
                "lamproclist create idx unsol "
                "{name1 {arg1 ...} name2 {arg1 ...} ...} or "
                "readoutlist create idx unsol "
                "name1 {arg1 ...} name2 {arg1 ...} ...",
                -1));
            return TCL_ERROR;
    }

    int idx, unsol;
    if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetInt(interp, argv[4], &unsol)!=TCL_OK)
        return TCL_ERROR;

    int res;
    if (argc==6) { // procedures are given as TCL list
        const char** mlv;
        int mlc;
        if (Tcl_SplitList(interp, argv[5], &mlc, &mlv)!=TCL_OK) {
            res=-1;
        } else {
            if (mlc%2) {
                Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # elements in list", -1));
                Tcl_Free((char*)mlv);
                return TCL_ERROR;
            }
            res=e_lamproclist_setup(mlc, mlv);
            Tcl_Free((char*)mlv);
        }
    } else {       // procedures are given as pairs of proc and args
        res=e_lamproclist_setup(argc-5, argv+5);
    }

    if (res<0)
        return TCL_ERROR;

    try {
        DownloadLamproc(idx, unsol);
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
        res=TCL_OK;
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        res=TCL_ERROR;
    }
    return res;
}
/*****************************************************************************/

int E_ved::e_lamproclist_delete(int argc, const char* argv[])
{
// <VED> lamproclist delete idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lamproclist delete idx",
      -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  DeleteLamproc(idx);
  if (confmode_==asynchron)
    {
    confback.xid(last_xid_);
    installconfback(confback);
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

int E_ved::e_lamproclist_upload(int argc, const char* argv[])
{
// <VED> lamproclist upload idx
//
confbackbox confback;
if (confmode_==asynchron)
  {
  if (confback.init(interp, this, argc, argv, &E_ved::f_lamupload)!=TCL_OK)
      return TCL_ERROR;
  }

if (argc!=4)
  {
  Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args; must be lamproclist upload idx",
      -1));
  return TCL_ERROR;
  }
int idx;
if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK) return TCL_ERROR;
try
  {
  if (confmode_==asynchron)
    {
    (void)UploadLamproc(idx);
    confback.xid(last_xid_);
    installconfback(confback);
    ostringstream ss;
    ss << last_xid_;
    Tcl_SetResult_Stream(interp, ss);
    }
  else
    {
    C_confirmation* conf=UploadLamproc(idx);
    ostringstream ss;
    f_lamupload(ss, conf, 0, 0, 0);
    Tcl_SetResult_Stream(interp, ss);
    delete conf;
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

int E_ved::e_trigger(int argc, const char* argv[])
//trigger
{
// <VED> trigger delete [idx]
// <VED> trigger [upload idx]
// <VED> trigger download [idx] proc [arg ...]
//
int res;
if (argc==2)
  res=e_trig_upload(argc, argv);
else
  {
  const char* names[]={
    "create",
    "delete",
    "upload",
    0};
  switch (findstring(interp, argv[2], names))
    {
    case 0: res=e_trig_create(argc, argv); break;
    case 1: res=e_trig_delete(argc, argv); break;
    case 2: res=e_trig_upload(argc, argv); break;
    default: res=TCL_ERROR; break;
    }
  }
return res;
}

/*****************************************************************************/
int E_ved::e_trig_upload(int argc, const char* argv[])
{
    // <VED> trigger [upload idx]
    confbackbox confback;
    int res, idx=0;

    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_trigger)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc!=2 && argc!=4) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args, must be "
                "trigger [upload idx]", -1));
        return TCL_ERROR;
    }
    if (argc>3) {
        if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK)
            return TCL_ERROR;
    }

    try {
        if (confmode_==synchron) {
            C_confirmation* conf=UploadTrigger(idx);
            ostringstream ss;
            f_trigger(ss, conf, 0, 0, 0);
            Tcl_SetResult_Stream(interp, ss);
            delete conf;
        } else {
            UploadTrigger(idx);
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
        res=TCL_OK;
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    return res;
}
/*****************************************************************************/
int E_ved::e_trig_delete(int argc, const char* argv[])
{
    // <VED> trigger delete
    confbackbox confback;
    int idx=0;

    if (confmode_==asynchron) {
        if (confback.init(interp, this, argc, argv, &E_ved::f_void)!=TCL_OK)
            return TCL_ERROR;
    }

    if (argc!=3 && argc!=4) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args, must be "
                "trigger delete [idx]", -1));
        return TCL_ERROR;
    }
    if (argc>3) {
        if (Tcl_GetInt(interp, argv[3], &idx)!=TCL_OK)
                return TCL_ERROR;
    }

    try {
        DeleteTrigger(idx);
        if (confmode_==asynchron) {
            confback.xid(last_xid_);
            installconfback(confback);
            ostringstream ss;
            ss << last_xid_;
            Tcl_SetResult_Stream(interp, ss);
        }
    } catch (C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }

    return TCL_OK;
}
/*****************************************************************************/
int E_ved::e_trig_create(int argc, const char* argv[])
{
    // <VED> trigger create [idx] proc {args}
    int res, n, idx;
    long val;

    if (argc<4) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args, must be "
                "trigger create [idx] proc {args}", -1));
        return TCL_ERROR;
    }

    n=3; // position of idx or proc
    /* is argv[n] a number? */
    if (isdigit(argv[n][0])) {
        if (Tcl_GetInt(interp, argv[n], &idx)!=TCL_OK)
            return TCL_ERROR;
        n++;   /* yes, integer --> idx*/
    } else {
        idx=0; /* no, must be the  proc then*/
    }

    if (argc<=n) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args, must be "
                "trigger create idx proc {args}", -1));
        return TCL_ERROR;
    }

    try {
        clear_triglist();
        trig_set_proc(argv[n++]);   // name of trigger procedure
        for (; n<argc; n++) {
            if (argv[n][0]=='\'') { // it is a string
                char* s=(char*)argv[n];
                char* se=s+strlen(s)-1;
                if (*se!='\'') {
                    ostringstream ss;
                    ss << "Unmatched ' in " << s;
                    Tcl_SetResult_Stream(interp, ss);
                    clear_triglist();
                    return TCL_ERROR;
                } else {
                    *se=0;
                    trig_add_param(s+1);
                    *se='\'';
                }
            } else {
                if (xTclGetLong(interp, (char*)argv[n], &val)!=TCL_OK) {
                    return TCL_ERROR;
                }
                trig_add_param((int)val);
            }
        }
        DownloadTrigger(idx);
        res=TCL_OK;
    } catch(C_ptr_error* e) {
        ostringstream ss;
        ss << "ved has no trigger procedures";
        Tcl_SetResult_Stream(interp, ss, e);
        delete e;
        return TCL_ERROR;
    } catch(C_error* e) {
        Tcl_SetResult_Err(interp, e);
        delete e;
        return TCL_ERROR;
    }
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
