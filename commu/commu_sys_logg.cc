/*
 * commu_logg.hxx
 * 
 * created before 05.02.95
 * 31.08.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <syslog.h>
#include "commu_logg.hxx"
#include <compat.h>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_sys_logg.cc,v 2.12 2014/07/14 15:12:20 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_syslogger::C_syslogger(const char* progname)
:C_logger(progname)
{
#if defined(__osf__) || defined(__NetBSD__) || defined(__SVR4) || defined(__linux__) // __SVR4 soll solaris bedeuten
  openlog(name, LOG_PID|LOG_CONS|LOG_NDELAY, LOG_USER);
#else
  openlog(name, LOG_PID);
#endif
}

/*****************************************************************************/

C_syslogger::~C_syslogger()
{
closelog();
}

/*****************************************************************************/

void C_syslogger::put(int prior, const char *s)
{
syslog(prior, "%s", s);
}

/*****************************************************************************/

void C_syslogger::put(int prior, const string& s)
{
    syslog(prior, "%s", s.c_str());
}

/*****************************************************************************/
void C_syslogger::lstring_t(int prior, const string& s)
{
syslog(prior, "%s", s.c_str());
}
/*****************************************************************************/

void C_syslogger::lstring_t(const char *s)
{
put(0, s);
}

/*****************************************************************************/

void C_syslogger::lstring_t(int prior, const char *s)
{
put(prior, s);
}

/*****************************************************************************/
/*****************************************************************************/
