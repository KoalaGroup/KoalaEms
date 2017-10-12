#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <unistd.h>
//#include "membug.hxx"

using namespace std;
#define GUARD 16

struct memobj {
    void* start;
    size_t size;
    int count;
};

static memobj* memlist=0;
static int memlistsize=0;
static int memlistused=0;

static int membug_find(void* p)
{
    int i;
    if (!memlist) return -1;

    for (i=memlistused-1; i && memlist[i].start!=p; i--);
    if (memlist[i].start==p) {
        return i;
    } else {
        return -1;
    }
}

void* membug_malloc(size_t size)
{
    void* p;
    int idx;

    p=malloc(size+GUARD);
    if (!p) return p;

    idx=membug_find(p);
    if (idx<0) {
        if (memlistsize==memlistused) {
            if (!memlistsize) {
                memlistsize=1024;
            } else {
                memlistsize*=2;
                cerr<<"membug_listsize="<<memlistsize<<endl;
            }
            memlist=(memobj*)realloc(memlist, memlistsize);
            if (!memlist) {
                cerr<<"membug_alloc: no memory!"<<endl;
                exit(0);
            }
        }
        idx=memlistused++;
    }
    memlist[idx].start=p;
    memlist[idx].size=size;
    memlist[idx].count=1;

    return (void*)((char*)p+GUARD);
}

void membug_free(void* p)
{
    int idx;
    if (!p) return;

    p=(void*)((char*)p-GUARD);
    idx=membug_find(p);
    if (idx<0) {
        cerr<<"free "<<p<<": not allocated"<<endl;
    } else if (memlist[idx].count!=1) {
        cerr<<"free "<<p<<": count="<<memlist[idx].count<<endl;
        memlist[idx].count--;
    } else {
        free(p);
        memset(p, 0, memlist[idx].size);
        memlist[idx].count--;
    }
}
#if 1
void* operator new(std::size_t size) throw (std::bad_alloc)
{
    return membug_malloc(size);
}
void* operator new[](std::size_t size) throw (std::bad_alloc)
{
    return membug_malloc(size);
}
void operator delete(void* p) throw()
{
    return membug_free(p);
}
void operator delete[](void* p) throw()
{
    return membug_free(p);
}
void* operator new(std::size_t size, const std::nothrow_t&) throw()
{
    return membug_malloc(size);
}
void* operator new[](std::size_t size, const std::nothrow_t&) throw()
{
    return membug_malloc(size);
}
void operator delete(void* p, const std::nothrow_t&) throw()
{
    return membug_free(p);
}
void operator delete[](void* p, const std::nothrow_t&) throw()
{
    return membug_free(p);
}
#else
void* operator new(std::size_t size)
{
    return membug_alloc(size);
}
void* operator new[](std::size_t size)
{
    return membug_alloc(size);
}
void operator delete(void* p) throw()
{
    return membug_free(p);
}
void operator delete[](void* p) throw()
{
    return membug_free(p);
}
#endif
