 /*
 * commu_list.cc
 * 
 * created 28.07.94 PW
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 * 15.06.1998 PW: rewritten
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <stdlib.h>
#include "commu_list_t.hxx"
#include "versions.hxx"

VERSION("Jun 15 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_list.cc,v 2.16 2004/11/26 15:14:19 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
ostream& C_int::print(ostream& os) const
{
os<<a;
/*if (!valid) os<<"nv";*/
return os;
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const C_int& rhs)
{
return rhs.print(os);
}
/*****************************************************************************/
ostream& C_intpair::print(ostream& os) const
{
os<<'{'<<a<<", "<<b<<'}';
/*if (!valid) os<<"nv";*/
return os;
}
/*****************************************************************************/
ostream& operator <<(ostream& os, const C_intpair& rhs)
{
return rhs.print(os);
}
/*****************************************************************************/
C_ints::C_ints(int size, STRING name)
:C_list<C_int>(size, name)
{}
/*****************************************************************************/
C_ints::C_ints(const C_ints& in)
:C_list<C_int>(in.growsize, in.name_)
{
//for (int i=0; i<firstfree; i++) insert(new C_int(*list[i]));
for (int i=0; i<in.firstfree; i++)
  {
  //C_int& ini=in[i];
  insert(new C_int(*in[i]));
  }
}
/*****************************************************************************/
int C_ints::get_idx(int a)
{
int i=0;
while ((i<firstfree) && (list[i]->a!=a)) i++;
if (i<firstfree)
  return i;
else
  return -1;
}
/*****************************************************************************/
void C_ints::free(int a)
{
int i=get_idx(a);
if (i>=0) remove_delete(i);
}
/*****************************************************************************/
int C_ints::exists(int a)
{
return (get_idx(a)>=0);
}
/*****************************************************************************/
void C_ints::add(int a)
{
insert(new C_int(a));
}
/*****************************************************************************/
/*****************************************************************************/
