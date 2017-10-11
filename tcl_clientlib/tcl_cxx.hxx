/*
 * tcl_cxx.hxx
 *
 * $ZEL: tcl_cxx.hxx,v 1.7 2014/09/10 18:02:01 wuestner Exp $
 *
 * created 14.Apr.2003
 */

#ifndef _tcl_cxx_hxx_
#define _tcl_cxx_hxx_

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <ems_errors.hxx>

using namespace std;

void set_error_code(Tcl_Interp* interp, const C_error* e);

void Tcl_SetResult_Err(Tcl_Interp* interp, const C_error* e);
void Tcl_SetResult_String(Tcl_Interp* interp, const string& s);
//void Tcl_SetResult_String(Tcl_Interp* interp, const string& s, const C_error* e);
void Tcl_SetResult_Stream(Tcl_Interp* interp, ostringstream& s);
void Tcl_SetResult_Stream(Tcl_Interp* interp, ostringstream& s, const C_error* e);

int Tcl_NoArgs(Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]);

void Tcl_AppendElement_Stream(Tcl_Interp* interp, ostringstream& s);

int Tcl_GetInt_u32(Tcl_Interp *interp, const char *str, ems_u32 *Ptr);
int Tcl_GetIntFromObj_u32(Tcl_Interp *interp, Tcl_Obj *objPtr, ems_u32 *Ptr);

int xTclGetLong(Tcl_Interp *interp, const char *string, long *longPtr);


#endif
