/*
 * linkdate.cc
 * 
 * created 18.01.96 PW
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <linkdate.hxx>
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: linkdate.cc,v 2.6 2014/07/14 15:12:20 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

const char* linkdate=__DATE__ " " __TIME__;

void printlinkdate(ostream& os)
{
os << endl << linkdate << endl;
}


/*****************************************************************************/
/*****************************************************************************/
