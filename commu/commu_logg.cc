/*
 * commu_logg.cc
 * 
 * created before 14.06.94
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <cstring>
#include <sys/time.h>
#include "commu_logg.hxx"
#include <compat.h>
#include "versions.hxx"

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_logg.cc,v 2.15 2009/08/21 21:50:51 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_logger::C_logger(const char* progname)
{
name=new char[strlen(progname)+1];
strcpy(name, progname);
}

/*****************************************************************************/

C_logger::~C_logger()
{
delete[] name;
}

/*****************************************************************************/

void C_logger::lstring_t(int prior, const char *s)
{
char ss[1024];
OSTRINGSTREAM st;

struct timeval tp;
gettimeofday(&tp, 0);
strftime(ss, 1024, "%H:%M:%S", localtime((time_t *)&tp.tv_sec));

st<<name<<' '<<ss<<' '<<s<<ends;
put(prior, st.str());
}

/*****************************************************************************/

void C_logger::lstring_t(int prior, const STRING &s)
{
char ss[1024];
OSTRINGSTREAM st;

struct timeval tp;
gettimeofday(&tp, 0);
strftime(ss, 1024, "%H:%M:%S", localtime((time_t *)&tp.tv_sec));

st<<name<<' '<<ss<<' '<<s<<ends;
put(prior, st.str());
}

/*****************************************************************************/

void C_logger::lstring(int prior, const char *s)
{
put(prior, s);
}

/*****************************************************************************/

void C_logger::lstring(int prior, const STRING& s)
{
put(prior, s.c_str());
}

/*****************************************************************************/
/*****************************************************************************/
