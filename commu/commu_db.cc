/*
 * commu_db.cc
 * 
 * created before: 06.08.94
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errors.hxx>
#include <commu_db.hxx>
#include "versions.hxx"

VERSION("Mar 26 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_db.cc,v 2.11 2004/11/26 15:14:16 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

void C_db::new_db(const STRING&)
{
throw new C_program_error("C_db::new_db not implemented");
}

/*****************************************************************************/

void C_db::setVED(const STRING&, const C_VED_addr&)
{
throw new C_program_error("C_db::changeVED not implemented");
}

/*****************************************************************************/
/*****************************************************************************/
