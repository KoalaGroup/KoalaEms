/*
 * linkdate.cc
 * 
 * created 18.01.96 PW
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <linkdate.hxx>
#include <versions.hxx>

VERSION("Jan 18 1996", __FILE__, __DATE__, __TIME__,
"$ZEL: linkdate.cc,v 2.5 2004/11/26 15:14:34 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

const char* linkdate=__DATE__ " " __TIME__;

void printlinkdate(ostream& os)
{
os << endl << linkdate << endl;
}


/*****************************************************************************/
/*****************************************************************************/
