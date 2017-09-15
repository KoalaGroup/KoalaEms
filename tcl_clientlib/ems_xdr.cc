/*
 * ems_xdr.cc
 * created 25.02.99 PW
 */

#include "config.h"
#include <errno.h>
#include <string.h>
#include <tcl.h>
#include "ems_xdr.hxx"
#include <xdrstring.h>
#include <versions.hxx>

VERSION("Nov 16 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: ems_xdr.cc,v 1.7 2010/06/20 22:15:59 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

int XDR2floatcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[])
{
// for (int i=0; i<objc; i++)
//   {
//   cerr<<"["<<i<<"] >"<<Tcl_GetString(objv[i])<<"<"<<endl;
//   }
    if (objc!=2) {
        Tcl_WrongNumArgs(interp, 1, objv, "int");
        return TCL_ERROR;
    }
    long l;
    union {
        int i;
        float f;
    } u;
    if (Tcl_GetLongFromObj(interp, objv[1], &l)!=TCL_OK)
        return TCL_ERROR;
    u.i=l;
    Tcl_Obj* obj=Tcl_NewDoubleObj(u.f);
    Tcl_SetObjResult(interp, obj);
    return TCL_OK;
}

int float2XDRcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[])
{
    if (objc!=2) {
        Tcl_WrongNumArgs(interp, 1, objv, "float");
        return TCL_ERROR;
    }

    double d;
    union {
        int i;
        float f;
    } u;
    if (Tcl_GetDoubleFromObj(interp, objv[1], &d)!=TCL_OK)
        return TCL_ERROR;
    u.f=d;
    Tcl_Obj* obj=Tcl_NewIntObj(u.i);
    Tcl_SetObjResult(interp, obj);
    return TCL_OK;
}
///////////////////////////////////////////////////////////////////////////////
int XDR2doublecommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[])
{
if (objc!=3)
  {
  Tcl_WrongNumArgs(interp, 1, objv, "int int");
  return TCL_ERROR;
  }
long l;
union
  {
  int i[2];
  double f;
  } u;
if (Tcl_GetLongFromObj(interp, objv[1], &l)!=TCL_OK) return TCL_ERROR;
u.i[1]=l;
if (Tcl_GetLongFromObj(interp, objv[2], &l)!=TCL_OK) return TCL_ERROR;
u.i[0]=l;
Tcl_Obj* obj=Tcl_NewDoubleObj(u.f);
Tcl_SetObjResult(interp, obj);
return TCL_OK;
}

int double2XDRcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[])
{
if (objc!=2)
  {
  Tcl_WrongNumArgs(interp, 1, objv, "double");
  return TCL_ERROR;
  }
union
  {
  int i[2];
  double f;
  } u;
if (Tcl_GetDoubleFromObj(interp, objv[1], &u.f)!=TCL_OK) return TCL_ERROR;

Tcl_Obj* obj[2];
obj[1]=Tcl_NewIntObj(u.i[0]);
obj[0]=Tcl_NewIntObj(u.i[1]);
Tcl_Obj* lobj=Tcl_NewListObj(2, obj);
Tcl_SetObjResult(interp, lobj);
return TCL_OK;
}
///////////////////////////////////////////////////////////////////////////////
/*
 * objv[1]: integer list (XDR)
 * objv[2]: name of variable for string
 * return: rest of list
 */
int XDR2stringcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[])
{
    if (objc!=3)    {
        Tcl_WrongNumArgs(interp, 1, objv, "list varname");
        return TCL_ERROR;
    }
    int listlen;
    if (Tcl_ListObjLength(interp, objv[1], &listlen)!=TCL_OK)
        return TCL_ERROR;
    if (listlen<1) {
        cerr<<"xdr2string: list is empty"<<endl;
        return TCL_ERROR;
    }

    ems_u32* ilist;
    int ilistlen;
    long lval;
    Tcl_Obj* obj;
    if (Tcl_ListObjIndex(interp, objv[1], 0, &obj)!=TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetLongFromObj(interp, obj, &lval)!=TCL_OK)
        return TCL_ERROR;

    ilistlen=(lval+3)/4+1;
    if (listlen<ilistlen || listlen<1) {
        cerr<<"xdr2string: list is too short"<<endl;
        return TCL_ERROR;
    }
    ilist=(ems_u32*)malloc(ilistlen*sizeof(ems_u32));
    if (ilist==(ems_u32*)0) {
        cerr<<"malloc: "<<strerror(errno)<<endl;
        return TCL_ERROR;
    }
    ilist[0]=lval;
    for (int i=1; i<ilistlen; i++) {
        if (Tcl_ListObjIndex(interp, objv[1], i, &obj)!=TCL_OK)
            return TCL_ERROR;
        if (Tcl_GetLongFromObj(interp, obj, &lval)!=TCL_OK)
            return TCL_ERROR;
        ilist[i]=lval;
    }
    char* str;
    xdrstrcdup(&str, ilist);
    free(ilist);
    if (Tcl_SetVar(interp, Tcl_GetString(objv[2]), str, TCL_LEAVE_ERR_MSG)==0) {
        free(str);
        return TCL_ERROR;
    }
    free(str);
    Tcl_Obj* res_obj;
    if (Tcl_IsShared(objv[1])) {
        res_obj=Tcl_DuplicateObj(objv[1]);
    } else {
        res_obj=objv[1];
    }
    Tcl_ListObjReplace(interp, res_obj, 0, ilistlen, 0, (Tcl_Obj**)0);
    Tcl_SetObjResult(interp, res_obj);
    return TCL_OK;
}

/*
 * objv[1]: string
 * return: integer list (XDR)
 */
int string2XDRcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[])
{
if (objc!=2)
  {
  Tcl_WrongNumArgs(interp, 1, objv, "string");
  return TCL_ERROR;
  }
ems_u32* ilist;
int ilistlen;
char* str=Tcl_GetString(objv[1]);
ilistlen=strxdrlen(str);
ilist=(ems_u32*)malloc(ilistlen*sizeof(ems_u32));
if (ilist==(ems_u32*)0)
  {
  cerr<<"malloc: "<<strerror(errno)<<endl;
  return TCL_ERROR;
  }
outstring(ilist, str);
Tcl_Obj* res_obj=Tcl_NewListObj(0, (Tcl_Obj**)0);
for (int i=0; i<ilistlen; i++)
  {
  Tcl_Obj* obj=Tcl_NewIntObj(ilist[i]);
  Tcl_ListObjAppendElement(interp, res_obj, obj);
  }
free(ilist);
Tcl_SetObjResult(interp, res_obj);
return TCL_OK;
}

/*****************************************************************************/
/*****************************************************************************/
