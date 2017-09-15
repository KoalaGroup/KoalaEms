/*
 * debug.cc
 * 
 * 19.07.93 PW
 */

#include <stdio.h>
#include <commu_log.hxx>
#include "versions.hxx"

VERSION("Jul 19 1993", __FILE__, __DATE__, __TIME__,
"$ZEL: debug.cc,v 2.5 2004/11/26 15:14:33 wuestner Exp $")
#define XVERSION

extern C_log elog, nlog, dlog;
static int rr=0;

/*****************************************************************************/

void trace(char *s)
{
elog << "trace " << rr++ << " -> " << s << flush;
}

/*****************************************************************************/

void trace_(char *s)
{
elog << "trace " << --rr << " <- " << s << flush;
}

/*****************************************************************************/
/*****************************************************************************/

