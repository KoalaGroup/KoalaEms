/*
 * emsAppInit.cc
 * created 01.09.95 PW
 * 13.11.2000 PW: extern "C" for Ems_AppInit
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <tcl.h>
#ifdef USE_TK
#include <tk.h>
#endif
#include <errors.hxx>
#include "emstcl.hxx"
#include <time.hxx>
#include "tcl_cxx.hxx"
#include "versions.hxx"

VERSION("Nov 16 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: emsAppInit.cc,v 1.12 2004/11/26 23:03:40 wuestner Exp $")
#define XVERSION

/*
 *----------------------------------------------------------------------
 *
 * Ems_AppInit --
 *
 *      This procedure performs application-specific initialization.
 *      Most applications, especially those that incorporate additional
 *      packages, will have their own version of this procedure.
 *
 * Results:
 *      Returns a standard Tcl completion code, and leaves an error
 *      message in interp->result if an error occurs.
 *
 * Side effects:
 *      Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

extern "C" {
int Ems_AppInit(Tcl_Interp* interp)
{
    if (Tcl_Init(interp) == TCL_ERROR)
        return TCL_ERROR;
#ifdef USE_TK
    if (Tk_Init(interp) == TCL_ERROR)
        return TCL_ERROR;
#endif
    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */

    if (Ems_Init(interp) == TCL_ERROR)
        return TCL_ERROR;
    if (Time_Init(interp) == TCL_ERROR)
        return TCL_ERROR;

    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

#ifdef USE_TK
    Tcl_SetVar(interp, "tcl_rcFileName", "~/.emswishrc", TCL_GLOBAL_ONLY);
#else
    Tcl_SetVar(interp, "tcl_rcFileName", "~/.emsshrc", TCL_GLOBAL_ONLY);
#endif
    return TCL_OK;
}
} /* extern "C" */

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      This is the main program for the application.
 *
 * Results:
 *      None: Tcl_Main never returns here, so this procedure never
 *      returns either.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int main(int argc, char* argv[])
{
    try {
#ifdef USE_TK
        Tk_Main(argc, argv, Ems_AppInit);
#else
        Tcl_Main(argc, argv, Ems_AppInit);
#endif
        return 0;                /* Needed only to prevent compiler warning. */
  } catch (int i) {
    cerr << "caught int << i << in main" << endl;
  }
// catch (...)
//   {
//   cerr << "caught something unknown in main" << endl;
//   }
}

/*****************************************************************************/
/*****************************************************************************/
