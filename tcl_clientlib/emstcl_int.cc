/*
 * tcl_cxx.cc
 *
 * created 2003-??-??
 */

#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <ctype.h>
#include <tcl.h>
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2014-07-10", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl_int.cc,v 1.3 2014/09/10 18:00:49 wuestner Exp $")
#define XVERSION

int
xTclGetLong(Tcl_Interp *interp, const char *string, long *longPtr)
{
    char *end, *p;
    long i;

    /*
     * Note: don't depend on strtoul to handle sign characters; it won't
     * in some implementations.
     */

    errno = 0;
    for (p = const_cast<char*>(string); isspace(*p); p++) {
	/* Empty loop body. */
    }
    if (*p == '-') {
	p++;
	i = -(int)strtoul(p, &end, 0);
    } else if (*p == '+') {
	p++;
	i = strtoul(p, &end, 0);
    } else {
	i = strtoul(p, &end, 0);
    }
    if (end == p) {
	badInteger:
        if (interp != (Tcl_Interp *) NULL) {
            Tcl_AppendResult(interp, "expected integer but got \"", string,
                    "\"", (char *) NULL);
        }
	return TCL_ERROR;
    }
    if (errno == ERANGE) {
        if (interp != (Tcl_Interp *) NULL) {
	    Tcl_SetResult(interp,
                    const_cast<char*>("integer value too large to represent"),
		    TCL_STATIC);
            Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
                    Tcl_GetStringResult(interp), (char *) NULL);
        }
	return TCL_ERROR;
    }
    while ((*end != '\0') && isspace(*end)) {
	end++;
    }
    if (*end != 0) {
	goto badInteger;
    }
    *longPtr = i;
    return TCL_OK;
}
