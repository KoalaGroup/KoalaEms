/*
 * histoAppInit.cc
 * 
 * created 19.02.96 PW
 */

#include <tcl.h>
#include <tk.h>
#include <histotcl.hxx>
#include <versions.hxx>

VERSION("2014-07-10", __FILE__, __DATE__, __TIME__,
"$ZEL: histoAppInit.cc,v 1.5 2014/07/10 18:22:24 wuestner Exp $")
#define XVERSION

/*
 *----------------------------------------------------------------------
 *
 * Histo_AppInit --
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

static int
Histo_AppInit(Tcl_Interp* interp)
{
if (Tcl_Init(interp) == TCL_ERROR) return TCL_ERROR;
if (Tk_Init(interp) == TCL_ERROR) return TCL_ERROR;
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

if (Histo_Init(interp) == TCL_ERROR) return TCL_ERROR;

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

Tcl_SetVar(interp, "tcl_rcFileName", "~/.histoshrc", TCL_GLOBAL_ONLY);
return TCL_OK;
}

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
Tk_Main(argc, argv, Histo_AppInit);
return 0;                   /* Needed only to prevent compiler warning. */
}

/*****************************************************************************/
/*****************************************************************************/
