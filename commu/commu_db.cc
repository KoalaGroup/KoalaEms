/*
 * commu_db.cc
 * 
 * created before: 06.08.94
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errors.hxx>
#include <commu_db.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_db.cc,v 2.12 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

void C_db::new_db(const string&)
{
throw new C_program_error("C_db::new_db not implemented");
}

/*****************************************************************************/

void C_db::setVED(const string&, const C_VED_addr&)
{
throw new C_program_error("C_db::changeVED not implemented");
}

/*****************************************************************************/
/*****************************************************************************/
