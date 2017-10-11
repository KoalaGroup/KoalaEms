/*
 * commu_logg.cc
 * 
 * created before 14.06.94
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <sys/time.h>
#include "commu_logg.hxx"
#include <compat.h>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_logg.cc,v 2.16 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

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
ostringstream st;

struct timeval tp;
gettimeofday(&tp, 0);
strftime(ss, 1024, "%H:%M:%S", localtime((time_t *)&tp.tv_sec));

st<<name<<' '<<ss<<' '<<s<<ends;
put(prior, st.str());
}

/*****************************************************************************/

void C_logger::lstring_t(int prior, const string &s)
{
char ss[1024];
ostringstream st;

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

void C_logger::lstring(int prior, const string& s)
{
put(prior, s.c_str());
}

/*****************************************************************************/
/*****************************************************************************/
