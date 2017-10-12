#ifndef _membug_hxx_
#define _membug_hxx_

extern "C" {
void* membug_malloc(size_t size);
void membug_free(void* p);
}

/*
using namespace std;
void* operator new(std::size_t size) throw (std::bad_alloc);
void* operator new[](std::size_t size) throw (std::bad_alloc);
void operator delete(void* p) throw();
void operator delete[](void* p) throw();
void* operator new(std::size_t size, const std::nothrow_t&) throw();
void* operator new[](std::size_t size, const std::nothrow_t&) throw();
void operator delete(void* p, const std::nothrow_t&) throw();
void operator delete[](void* p, const std::nothrow_t&) throw();
*/

#endif
