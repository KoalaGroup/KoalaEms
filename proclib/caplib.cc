/*
 * caplib.cc
 * 
 * created: 16.08.94 PW
 * 25.03.1998 PW: adapded for <string>
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#define _BSD_SOURCE

#include "config.h"
#include "cxxcompat.hxx"
#include <cerrno>
#include <stdlib.h>
#include <cstring>
#include <caplib.hxx>
#include <errors.hxx>
#include <versions.hxx>

VERSION("2008-Nov-15", __FILE__, __DATE__, __TIME__,
"$ZEL: caplib.cc,v 2.12 2010/02/02 23:57:54 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_capability_list::C_capability_list(int size)
:maxsize(size), entries(0), separator(DEF_SEPARATOR)
{
//cerr << "C_capability_list::C_capability_list(); this=" << (void*)this << endl;
caplist=new capentry[size];
if (caplist==0) throw new C_unix_error(errno, "Can't store capability list");
}

/*****************************************************************************/

C_capability_list::~C_capability_list()
{
//cerr << "C_capability_list::~C_capability_list(); this=" << (void*)this << endl;
int i;

for (i=0; i<entries; i++) delete[] caplist[i].name;
delete[] caplist;
}

/*****************************************************************************/

void C_capability_list::add(char* name, int index, int version)
{
if (entries==maxsize) throw 0;
caplist[entries].name=name;
caplist[entries].index=index;
caplist[entries].version=version;
entries++;
}

/*****************************************************************************/

int C_capability_list::get(const char* name) const
{
const char* spos;

if ((spos=rindex(name, separator))!=0)
  {
  int ver;
  int sidx;

  sidx=spos-name;
  if (sidx==0) return(-1);               // Name beginnt mit Separator
  ISTRINGSTREAM s(spos+1);
  if (s >> ver)
    {
    char *dummy, c;
    int res;

    dummy=new char[strlen(name)+1];
    if (s >> c) return(-1);          // nach der Version folgt Mist
    strncpy(dummy, name, sidx);
    dummy[sidx]=0;
    res=get(dummy, ver);
    delete[] dummy;
    return(res);
    }
  else
    return(-1);                          // Version ist keine Zahl
  }
else
  {
  int ver, idx, i;

  ver=-1;
  idx=-1;
  for (i=0; i<entries; i++)
    {
    if (strcmp(name, caplist[i].name)==0)
      {
      if (caplist[i].version>ver)
        {
        idx=caplist[i].index;
        ver=caplist[i].version;
        }
      }
    }
  return(idx);
  }
}

/*****************************************************************************/

int C_capability_list::get(const char* name, int version) const
{
int i;

for (i=0; i<entries; i++)
  {
  if (strcmp(name, caplist[i].name)==0)
    {
    if (caplist[i].version==version) return(caplist[i].index);
    }
  }
return(-1);
}

/*****************************************************************************/

STRING C_capability_list::get(int proc) const
{
int i;
i=0;
while ((i<entries) && (caplist[i].index!=proc)) i++;

if (caplist[i].index!=proc)
  {
  OSTRINGSTREAM ss;
  ss << "No procedure with index " << proc;
// #ifdef NO_ANSI_CXX
//   char* s=ss.str();
//   C_error* e=new C_program_error(s);
//   delete[] s;
// #else
//   C_error* e=new C_program_error(ss.str());
// #endif
  C_error* e=new C_program_error(ss);
  throw e;
  }

OSTRINGSTREAM ss;
STRING st;
ss << caplist[i].name << separator << caplist[i].version;
st=ss.str();
return st;
}

/*****************************************************************************/

int C_capability_list::size() const
{
return(entries);
}

/*****************************************************************************/

char C_capability_list::version_separator() const
{
return(separator);
}

/*****************************************************************************/

char C_capability_list::version_separator(char sep)
{
char c;
c=separator;
separator=sep;
return(c);
}

/*****************************************************************************/
/*****************************************************************************/
