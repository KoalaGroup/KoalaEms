/*
 * events++/memory.cc
 * created: 29.03.1998 PW
 * 11.02.1999 PW: operator new[] and operator delete[] added
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "memory.hxx"
#include <versions.hxx>

VERSION("Feb 11 1999", __FILE__, __DATE__, __TIME__,
"$ZEL: memory.cc,v 1.8 2004/11/26 14:40:20 wuestner Exp $")

#define XVERSION

int memory::used_mem=0;

/*****************************************************************************/
#ifdef HAVE_OP_NEW
#ifdef HAVE_BAD_ALLOC
void* operator new(size_t s) throw(std::bad_alloc)
#else
void* operator new(size_t s)
#endif
{
//fprintf(stderr, "operator new(%d)", s);
size_t* p=(size_t*)malloc(s+sizeof(size_t));
if (p) {
    memory::used_mem+=s;
    *p++=s;
}
return p;
}
/*****************************************************************************/
void operator delete(void* p) throw()
{
if (p)
  {
  size_t* pp=((size_t*)p)-1;
  size_t s=*pp;
  //fprintf(stderr, "operator delete(p=%p, s=%d, used_mem=\n",
  //    p, s, memory::used_mem);
  memory::used_mem-=s;
  free(pp);
  }
//else
  //fprintf(stderr, "operator delete(0)\n");
}
#endif
/*****************************************************************************/
#ifdef HAVE_OP_NEW_ARR
#ifdef HAVE_BAD_ALLOC
void* operator new[](size_t s) throw(std::bad_alloc)
#else
void* operator new[](size_t s)
#endif
{
//fprintf(stderr, "operator new[](%d)", s);
size_t* p=(size_t*)malloc(s+sizeof(size_t));
if (p) {
    memory::used_mem+=s;
    *p++=s;
}
return p;
}
/*****************************************************************************/
void operator delete[](void* p) throw()
{
if (p)
  {
  size_t* pp=((size_t*)p)-1;
  size_t s=*pp;
  //fprintf(stderr, "operator delete[](p=%p, s=%d, used_mem=\n",
  //    p, s, memory::used_mem);
  memory::used_mem-=s;
  free(pp);
  }
//else
  //fprintf(stderr, "operator delete[](0)\n");
}
#endif
/*****************************************************************************/
/*****************************************************************************/
