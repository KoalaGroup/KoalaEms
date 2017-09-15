/*
 * commu_nonedb.cc
 * 
 * created 14.08.95 PW
 */

#include <errors.hxx>
#include <commu_nonedb.hxx>
#include "versions.hxx"

VERSION("Jun 05 1996", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_nonedb.cc,v 2.6 2004/11/26 15:14:25 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

void C_nonedb::all()
{
throw new C_program_error("no VED database available");
}

/*****************************************************************************/

char* C_nonedb::listname()
{
ostrstream ss;
ss << "no VED database" << ends;
return(ss.str());
}

/*****************************************************************************/
/*****************************************************************************/
