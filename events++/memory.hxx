/*
 * events++/memory.hxx
 * $ZEL: memory.hxx,v 1.5 2004/02/11 12:36:08 wuestner Exp $
 *
 * created: 29.03.1998 PW
 * 11.02.1999 PW: operator new[] and operator delete[] added
 */

#ifndef _memory_hxx_
#define _memory_hxx_

#include "config.h"

#include <sys/types.h>

#ifdef HAVE_NAMESPACES
namespace std{}
#endif

class memory
  {
#ifdef HAVE_OP_NEW
#ifdef HAVE_BAD_ALLOC
  friend void* operator new(size_t) throw(std::bad_alloc);
#else
  friend void* operator new(size_t);
#endif
  friend void operator delete(void*) throw();
#endif
#ifdef HAVE_OP_NEW_ARR
#ifdef HAVE_BAD_ALLOC
  friend void* operator new[](size_t) throw(std::bad_alloc);
#else
  friend void* operator new[](size_t);
#endif
  friend void operator delete[](void*) throw();
#endif
  public:
    static int used_mem;
  };

#ifdef HAVE_OP_NEW
#ifdef HAVE_BAD_ALLOC
  void* operator new(size_t) throw(std::bad_alloc);
#else
  void* operator new(size_t);
#endif
  void operator delete(void*) throw();
#endif
#ifdef HAVE_OP_NEW_ARR
#ifdef HAVE_BAD_ALLOC
  void* operator new[](size_t) throw(std::bad_alloc);
#else
void* operator new[](size_t);
#endif
  void operator delete[](void*) throw();
#endif

#endif
