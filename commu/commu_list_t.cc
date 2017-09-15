 /*
 * commu_list_t.cc
 * 
 * created 28.07.94 PW
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 * 15.06.1998 PW: rewritten
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <stdlib.h>
#include <debug.hxx>
#include "commu_list_t.hxx"
#include "versions.hxx"

#ifndef XVERSION
VERSION("Mar 26 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_list_t.cc,v 2.8 2004/11/26 15:14:20 wuestner Exp $")
#define XVERSION
#endif

/*****************************************************************************/
template<class T>
C_list<T>::C_list(int size, STRING name)
:name_(name), growsize(size), listsize(size), firstfree(0)
{
TR(C_list::C_list)
list=new T*[listsize];
}
/*****************************************************************************/
template<class T>
C_list<T>::~C_list()
{
int i;

TR(T_list::~T_list)
for (i=0; i<firstfree; i++) delete list[i];
delete[] list;
}
/*****************************************************************************/
template<class T>
void C_list<T>::growlist()
{
T **hlist;
int i;

TR(C_list::growlist)
if (listsize>firstfree)
  {
  return;
  }
hlist=new T*[listsize+growsize];
for (i=0; i<firstfree; i++) hlist[i]=list[i];
delete[] list;
list=hlist;
listsize+=growsize;
}
/*****************************************************************************/
template<class T>
void C_list<T>::shrinklist()
{
T **hlist;
int i;

TR(C_list::shrinklist)
if (listsize==growsize)
  {
  return;
  }
int nlistsize=(firstfree/growsize+2)*growsize;
if (listsize<=nlistsize)
  {
  return;
  }
listsize=nlistsize;
hlist=new T*[listsize];
for (i=0; i<listsize; i++) hlist[i]=list[i];
delete list;
list=hlist;
}
/*****************************************************************************/
template<class T>
void C_list<T>::insert(T* entry)
{
TR(C_ulist::insert)
growlist();
list[firstfree++]=entry;
counter();
}
/*****************************************************************************/
template<class T>
void C_list<T>::delete_all()
{
for (int i=0; i<firstfree; i++) delete list[i];
firstfree=0;
shrinklist();
counter();
}
/*****************************************************************************/
template<class T>
void C_list<T>::remove(int idx)
{
int j;
if (idx>=firstfree)
  {
  cerr<<"C_list<T>::remove_delete("<<idx<<"): index does not exist"<<endl;
  return;
  }
firstfree--;
for (j=idx; j<firstfree; j++) list[j]=list[j+1];
shrinklist();
counter();
}
/*****************************************************************************/
template<class T>
void C_list<T>::remove(T* entry)
{
int i, j;
i=0;
while ((i<firstfree) && (list[i]!=entry)) i++;
if (i==firstfree)
  {
  cerr<<"C_list<T>::remove_delete("<<(void*)entry<<"): entry does not exist"
      <<endl;
  return;
  }
firstfree--;
for (j=i; j<firstfree; j++) list[j]=list[j+1];
shrinklist();
counter();
}
/*****************************************************************************/
template<class T>
void C_list<T>::remove_delete(int idx)
{
int j;
if (idx>=firstfree)
  {
  cerr<<"C_list<T>::remove_delete("<<idx<<"): index does not exist"<<endl;
  return;
  }
delete list[idx];
firstfree--;
for (j=idx; j<firstfree; j++) list[j]=list[j+1];
shrinklist();
counter();
}
/*****************************************************************************/
template<class T>
void C_list<T>::remove_delete(T* entry)
{
int i, j;
i=0;
while ((i<firstfree) && (list[i]!=entry)) i++;
if (i==firstfree)
  {
  cerr<<"C_list<T>::remove_delete("<<(void*)entry<<"): entry does not exist"
      <<endl;
  return;
  }
delete list[i];
firstfree--;
for (j=i; j<firstfree; j++) list[j]=list[j+1];
shrinklist();
counter();
}
/*****************************************************************************/
template<class T>
T* C_list<T>::extract_first()
{
T* t;
if (firstfree==0) return (T*)0;
t=list[0];
firstfree--;
for (int i=0; i<firstfree; i++) list[i]=list[i+1];
shrinklist();
counter();
return t;
}
/*****************************************************************************/
template<class T>
T* C_list<T>::extract_last()
{
T* t;
if (firstfree==0) return (T*)0;
t=list[--firstfree];
shrinklist();
counter();
return t;
}
/*****************************************************************************/
template<class T>
ostream& C_list<T>::print(ostream& os) const
{
for (int i=0; i<firstfree; i++) os<<(*list[i])<<' ';
os<<endl;
return os;
}
/*****************************************************************************/
template <class T> ostream& operator <<(ostream& os, const C_list<T>& rhs)
{
return rhs.print(os);
}
/*****************************************************************************/
#ifdef __GNUC__
template class C_list<C_int>;
#else
#pragma define_template C_list<C_int>
#pragma define_template C_list<C_intpair>
#endif
/*****************************************************************************/
/*****************************************************************************/
