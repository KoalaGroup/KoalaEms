/*
 * smartptr_t.cc
 * 
 * created: 12.06.1998
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <stdlib.h>
#include "smartptr_t.hxx"
#include "proc_namelist.hxx"
#include "caplib.hxx"
#include "proc_plist.hxx"
#include <errors.hxx>
#include <versions.hxx>

#ifndef XVERSION
VERSION("Jun 12 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: smartptr_t.cc,v 1.8 2004/11/26 14:44:36 wuestner Exp $")
#define XVERSION
#endif

/*****************************************************************************/

template <class T>
T* T_smartptr<T>::operator ->()
{
if (p==0)
  {
  OSTRINGSTREAM os;
  os<<"smartptr("<<name<<")operator->; p="<<(void*)p;
  throw new C_ptr_error(os);
  }
return p;
}

/*****************************************************************************/

template <class T>
T_smartptr<T>::operator T*(void)
{
if (p==0)
  {
  OSTRINGSTREAM os;
  os<<"smartptr("<<name<<")cast to *; p="<<(void*)p<<endl;
  //xx=*(int*)0;
  throw new C_ptr_error(os);
  }
return p;
}

/*****************************************************************************/

template <class T>
T& T_smartptr<T>::operator *(void)
{
if (p==0)
  {
  OSTRINGSTREAM os;
  os<<"smartptr("<<name<<")operator*; p="<<(void*)p<<endl;
  throw new C_ptr_error(os);
  }
return *p;
}

/*****************************************************************************/
#ifdef __GNUC__
template class T_smartptr<C_namelist>; 
template class T_smartptr<C_capability_list>; 
template class T_smartptr<C_proclist>; 
#else
#pragma define_template T_smartptr<C_namelist>
#pragma define_template T_smartptr<C_capability_list>
#pragma define_template T_smartptr<C_proclist>
#endif

/*****************************************************************************/
/*****************************************************************************/
