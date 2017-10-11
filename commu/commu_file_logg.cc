/*
 * commu_file_logg.cc
 * 
 * created before 05.02.95 PW
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cerrno>
#include <cstring>
#include <commu_logg.hxx>
#include <errors.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_file_logg.cc,v 2.11 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_filelogger::C_filelogger(const char *progname, const char *logfile)
:C_logger(progname), dest(0)
{
    if ((logfile==0)
            || (strcmp(logfile, "")==0)
            || (strcmp(logfile, "cout")==0)) {
        dest=1;
    } else if (strcmp(logfile, "clog")==0) {
        dest=2;
    } else if (strcmp(logfile, "cerr")==0) {
        dest=3;
    } else {
        out.open(logfile);
        if (!out) {
            ostringstream ss;
            ss << "open logfile \"" << logfile << "\"" << ends;
            throw new C_unix_error(errno, ss);
        }
        dest=4;
    }
}

/*****************************************************************************/

C_filelogger::~C_filelogger()
{}

/*****************************************************************************/

void C_filelogger::put(int prior, const char *s)
{
switch (dest)
  {
  case 0: break;
  case 1: cout << s << endl; break;
  case 2: clog << s << endl; break;
  case 3: cerr << s << endl; break;
  case 4: out << s << endl; break;
  }
}

/*****************************************************************************/

void C_filelogger::put(int prior, const string& s)
{
    switch (dest) {
        case 0: break;
        case 1: cout << s << endl; break;
        case 2: clog << s << endl; break;
        case 3: cerr << s << endl; break;
        case 4: out << s << endl; break;
    }
}

/*****************************************************************************/
/*****************************************************************************/
