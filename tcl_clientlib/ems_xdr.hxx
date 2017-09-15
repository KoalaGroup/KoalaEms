/*
 * ems_xdr.hxx
 *
 * $ZEL: ems_xdr.hxx,v 1.3 2004/11/18 12:35:38 wuestner Exp $
 *
 * created 25.02.99 PW
 * 14.Nov.2000 PW: extern "C" for all procedures
 */

#ifndef _ems_xdr_hxx_
#define _ems_xdr_hxx_

#include "config.h"
#include <tcl.h>
extern "C" {
int XDR2floatcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[]);

int XDR2doublecommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[]);

int XDR2stringcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[]);

int float2XDRcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[]);

int double2XDRcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[]);

int string2XDRcommand(ClientData clientData, Tcl_Interp *interp, int objc,
    struct Tcl_Obj* const objv[]);
}
#endif
