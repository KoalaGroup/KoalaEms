/*
 * strlist.cc
 * 
 * created 21.01.95 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <string.h>
#include "stringlist.hxx"

#include "versions.hxx"

VERSION("Jun 06 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: stringlist.cc,v 2.7 2004/11/26 14:45:40 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_strlist::C_strlist(int size)
:num(size)
{
int i;

list=new char*[num];
valid=new int[num];
for (i=0; i<num; i++) list[i]=0;
for (i=0; i<num; i++) valid[i]=0;
}

/*****************************************************************************/

C_strlist::C_strlist(const C_strlist& sl)
:num(sl.num)
{
int i;

list=new char*[num];
valid=new int[num];
for (i=0; i<num; i++)
  if (sl.valid[i])
    {
    list[i]=new char[strlen(sl.list[i])+1];
    strcpy(list[i], sl.list[i]);
    valid[i]=1;
    }
  else
    valid[i]=0;
}

/*****************************************************************************/

C_strlist::~C_strlist()
{
int i;

for (i=0; i<num; i++) delete[] list[i];
delete[] list;
delete[] valid;
}

/*****************************************************************************/

void C_strlist::set(int i, const char *s)
{
if ((i<0) || (i>=num)) throw 0;
if (valid[i])
  {
  if (list[i]==s)
    return;
  else
    delete[] list[i];
  }
int ll=strlen(s)+1;
char* lp=new char[ll];
list[i]=lp;
strcpy(list[i], s);
valid[i]=1;
}

/*****************************************************************************/

const char* C_strlist::operator[](int i) const
{
if ((i<0) || (i>=num)) throw 0;
return list[i];
}

/*****************************************************************************/

int C_strlist::operator ==(const C_strlist& rhs) const
{
int i;

if (num!=rhs.num) return 0;
for (i=0; i<num; i++)
  {
  if (valid[i]!=rhs.valid[i]) return(0);
  if (valid[i] && strcmp(list[i], rhs.list[i])) return 0;
  }
return 1;
}

/*****************************************************************************/
/*****************************************************************************/
