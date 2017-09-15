/*
 * histotcl.cc
 * 
 * created 19.02.96
 */

#include <tcl.h>
#include "histotcl.hxx"

#include "histowin.hxx"
#include "newhistoarr.hxx"
#include <float.h>
#include <versions.hxx>

VERSION("Nov 17 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: histotcl.cc,v 1.7 2005/02/22 17:46:04 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

extern "C"
{
int Histo_Init(Tcl_Interp* interp)
{
#if 0
#ifdef __osf__
    write_rnd(FP_RND_RN); // Round toward nearest
#endif
#endif
    C_histoarrays* arrays=new C_histoarrays(interp);
    Tcl_CallWhenDeleted(interp, C_histoarrays::destroy, ClientData(arrays));

    E_histowin::inithints* hints=new E_histowin::inithints;
    hints->harrs=arrays;
    hints->mainwin=Tk_MainWindow(interp);
    Tcl_CallWhenDeleted(interp, E_histowin::inithints::destroy, ClientData(hints));

    Tcl_CreateObjCommand(interp, "histoarray", E_histoarray::create,
            (ClientData)arrays, (Tcl_CmdDeleteProc*)0);

    Tcl_CreateObjCommand(interp, "histowin", E_histowin::create,
            (ClientData)hints, (Tcl_CmdDeleteProc*)0);

    return TCL_OK;
}
}

/*****************************************************************************/
/*****************************************************************************/
