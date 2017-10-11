/*
 * time.cc
 * 
 * created 15.11.95 PW
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <tcl.h>
#include <errors.hxx>
#include "tcl_cxx.hxx"
#include "time.hxx"
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: time.cc,v 1.15 2014/07/14 15:13:26 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/
static int Tdoubletime(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=1) {
        return Tcl_NoArgs(interp, 1, objv);
    }
    struct timeval tv;
    gettimeofday(&tv, 0);
    ostringstream ss;
    ss << setprecision(20) << tv.tv_sec+(double)tv.tv_usec/1000000.0;
    Tcl_SetResult_Stream(interp, ss);
    return TCL_OK;
}
/*****************************************************************************/
static int Ttimeofday(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=1) {
        return Tcl_NoArgs(interp, 1, objv);
    }
    struct timeval tv;
    gettimeofday(&tv, 0);
    ostringstream ss;
    ss << tv.tv_sec << ' ' << tv.tv_usec;
    Tcl_SetResult_Stream(interp, ss);
    return TCL_OK;
}
/*****************************************************************************/
static int Ttimeofday_sec(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=1) {
        return Tcl_NoArgs(interp, 1, objv);
    }
    struct timeval tv;
    gettimeofday(&tv, 0);
    ostringstream ss;
    ss << tv.tv_sec;
    Tcl_SetResult_Stream(interp, ss);
    return TCL_OK;
}
/*****************************************************************************/
static int Tlocaltime(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=2) {
        Tcl_WrongNumArgs(interp, 1, objv, "time");
        return TCL_ERROR;
    }
    time_t time;
    int tt;
    if (Tcl_GetIntFromObj(interp, objv[1], &tt)!=TCL_OK)
        return TCL_ERROR;
    time=tt;
    struct tm* tm=localtime(&time);
    if (tm==0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj(strerror(errno), -1));
        return TCL_ERROR;
    } else {
        ostringstream ss;
        ss << tm->tm_sec <<' '<< tm->tm_min <<' '<< tm->tm_hour <<' '
            << tm->tm_mday <<' '<< tm->tm_mon <<' '<< tm->tm_year <<' '
            << tm->tm_wday <<' '<< tm->tm_yday <<' '<< tm->tm_isdst;
        Tcl_SetResult_Stream(interp, ss);
        return TCL_OK;
    }
}
/*****************************************************************************/
static int Tgmtime(ClientData clientdata, Tcl_Interp* interp, int objc,
    Tcl_Obj* const objv[])
{
    if (objc!=2) {
        Tcl_WrongNumArgs(interp, 1, objv, "time");
        return TCL_ERROR;
    }
    time_t time;
    int tt;
    if (Tcl_GetIntFromObj(interp, objv[1], &tt)!=TCL_OK) return TCL_ERROR;
    time=tt;
    struct tm* tm=gmtime(&time);
    if (tm==0) {
        Tcl_SetObjResult(interp,
            Tcl_NewStringObj(strerror(errno), -1));
        return TCL_ERROR;
    } else {
        ostringstream ss;
        ss << tm->tm_sec <<' '<< tm->tm_min <<' '<< tm->tm_hour <<' '
            << tm->tm_mday <<' '<< tm->tm_mon <<' '<< tm->tm_year <<' '
            << tm->tm_wday <<' '<< tm->tm_yday <<' '<< tm->tm_isdst;
        Tcl_SetResult_Stream(interp, ss);
        return TCL_OK;
    }
}
/*****************************************************************************/
extern "C"
{
    int Time_Init(Tcl_Interp* interp)
    {
    Tcl_CreateObjCommand(interp, "timeofday", Ttimeofday,
        ClientData(0), (Tcl_CmdDeleteProc*)0);
    Tcl_CreateObjCommand(interp, "timeofday_sec", Ttimeofday_sec,
        ClientData(0), (Tcl_CmdDeleteProc*)0);
    Tcl_CreateObjCommand(interp, "localtime", Tlocaltime,
        ClientData(0), (Tcl_CmdDeleteProc*)0);
    Tcl_CreateObjCommand(interp, "gmtime", Tgmtime,
        ClientData(0), (Tcl_CmdDeleteProc*)0);
    Tcl_CreateObjCommand(interp, "doubletime", Tdoubletime,
        ClientData(0), (Tcl_CmdDeleteProc*)0);

    return TCL_OK;
}}
/*****************************************************************************/
/*****************************************************************************/
