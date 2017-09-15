/*
 * tcl_cxx.cc
 *
 * created 14.Apr.2003
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include "cxxcompat.hxx"
#include <tcl.h>
#include <errors.hxx>
#include <ems_errors.hxx>
#include <ved_errors.hxx>
#include "emstcl_ved.hxx"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("Nov 16 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: tcl_cxx.cc,v 1.5 2006/02/16 20:57:15 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
void set_error_code(Tcl_Interp* interp, const C_error* e)
{
    int et=e->type();
    if (et==C_error::e_unknown) {
    } else if (et==C_error::e_none) {
    } else if (et==C_error::e_system) {
    } else if (et==C_error::e_program) {
    } else if (et==C_ems_error::e_ems) {
        C_ems_error* ee=(C_ems_error*)e;
        EMSerrcode error=ee->code();
        OSTRINGSTREAM ss;
        ss<<(int)error;
        STRING s=ss.str();
        Tcl_SetErrorCode(interp, "EMS", (char*)s.c_str(), (char*)0);
        {}
        // z.B. code 61 (Can't open VED "393": Connection refused)
    } else if (et==C_ved_error::e_ved) {
        C_ved_error* ee=(C_ved_error*)e;
        C_ved_error::errtypes eet=ee->errtype();
        switch (eet) {
        case C_ved_error::string_err:
            // z.B. unknown procedure "FRBD"
            break;
        case C_ved_error::sys_err:
            // z.B. Can't get confirmation: Connection timed out
            break;
        case C_ved_error::ems_err:
            break;
        case C_ved_error::req_err: {
            errcode error=ee->error().req_err;
            Tcl_Obj* errorObjPtr=Tcl_NewObj();
            {
                Tcl_Obj* StringObj=Tcl_NewStringObj("EMS_REQ", -1);
                Tcl_ListObjAppendElement(interp, errorObjPtr, StringObj);
            }
            {
                OSTRINGSTREAM ss;
                ss<<(int)error;
                STRING s=ss.str();
                Tcl_Obj* StringObj=Tcl_NewStringObj((char*)s.c_str(), s.length());
                Tcl_ListObjAppendElement(interp, errorObjPtr, StringObj);
            }
            const C_confirmation* conf=&ee->errconf();
            for (int i=1; i<conf->size(); i++) {
                OSTRINGSTREAM ss;
                ss<<conf->buffer(i);
                STRING s=ss.str();
                Tcl_Obj* StringObj=Tcl_NewStringObj((char*)s.c_str(), s.length());
                Tcl_ListObjAppendElement(interp, errorObjPtr, StringObj);
            }
            Tcl_SetObjErrorCode(interp, errorObjPtr);
            }
            break;
        case C_ved_error::pl_err:
            break;
        case C_ved_error::rt_err:
            break;
        default:
            break;
        }
    } else {
        cerr<<"set_error_code: error type="<<e->type()<<endl;
        cerr<<"error: "<<(*e)<<endl;
    }
}
/*****************************************************************************/
void Tcl_SetResult_Err(Tcl_Interp* interp, const C_error* e)
{
    OSTRINGSTREAM s;
    s<<(*e);
    STRING st=s.str();
    Tcl_SetResult(interp, (char*)st.c_str(), TCL_VOLATILE);
    set_error_code(interp, e);
}

void Tcl_SetResult_Stream(Tcl_Interp* interp, OSTRINGSTREAM& s,
        const C_error* e)
{
    STRING st=s.str();
    Tcl_SetObjResult(interp,
        Tcl_NewStringObj((char*)st.c_str(), st.length()));
    //Tcl_SetResult(interp, (char*)st.c_str(), TCL_VOLATILE);
    set_error_code(interp, e);
}

void Tcl_SetResult_Stream(Tcl_Interp* interp, OSTRINGSTREAM& s)
{
    STRING st=s.str();
    Tcl_SetObjResult(interp,
        Tcl_NewStringObj((char*)st.c_str(), st.length()));
    //Tcl_SetResult(interp, (char*)st.c_str(), TCL_VOLATILE);
}


void Tcl_SetResult_String(Tcl_Interp* interp, const STRING& s,
        const C_error* e)
{
    Tcl_SetObjResult(interp,
        Tcl_NewStringObj((char*)s.c_str(), s.length()));
    //Tcl_SetResult(interp, (char*)s.c_str(), TCL_VOLATILE);
    set_error_code(interp, e);
}

void Tcl_SetResult_String(Tcl_Interp* interp, const STRING& s)
{
    Tcl_SetObjResult(interp,
        Tcl_NewStringObj((char*)s.c_str(), s.length()));
    //Tcl_SetResult(interp, (char*)s.c_str(), TCL_VOLATILE);
}

void Tcl_AppendElement_Stream(Tcl_Interp* interp, OSTRINGSTREAM& s)
{
    STRING st=s.str();
    Tcl_AppendElement(interp, st.c_str());
}
/*****************************************************************************/
int
Tcl_NoArgs(Tcl_Interp* interp, int objc, Tcl_Obj* const objv[])
{
    Tcl_WrongNumArgs(interp, objc, objv, "");
    return TCL_ERROR;
}
/*****************************************************************************/
int
Tcl_GetInt_u32(Tcl_Interp *interp, const char *str, ems_u32 *Ptr)
{
    int res, val;
    res=Tcl_GetInt(interp, str, &val);
    *Ptr=(ems_u32)val;
    return res;
}
/*****************************************************************************/
int
Tcl_GetIntFromObj_u32(Tcl_Interp *interp, Tcl_Obj *objPtr, ems_u32 *Ptr)
{
    int res, val;
    res=Tcl_GetIntFromObj(interp, objPtr, &val);
    *Ptr=(ems_u32)val;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
