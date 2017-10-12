/*
 * support/anylist.cc
 * 
 * created 2014-09-10 PW
 */

//#include "config.h"
#include <typeinfo>
#include <string.h>
#include "anylist.hxx"

#include "versions.hxx"

VERSION("2014-09-10", __FILE__, __DATE__, __TIME__,
"$ZEL: anylist.cc,v 1.1 2014/09/12 13:06:20 wuestner Exp $")
#define XVERSION

C_anylist::C_anylist(int size)
:size_(size), num_(0)
{
    list=new value[size];
}

void
C_anylist::add(uint32_t val)
{
    if (num_==size_)
        enlarge();

    list[num_].type=&typeid(uint32_t);
    list[num_].val=new uint32_t(val);
    num_++;
}

void
C_anylist::add(const char *val)
{
    if (num_==size_)
        enlarge();

    list[num_].type=&typeid(char*);
    list[num_].val=strdup(val);
    num_++;
}

void
C_anylist::enlarge(void)
{
    value *nlist;

    size_*=2;
    nlist=new value[size_];
    for (int i=0; i<num_; i++)
        nlist[i]=list[i];
    delete[] list;
    list=nlist;
}

C_anylist::~C_anylist()
{
    for (int i=0; i<num_; i++)
        ::operator delete(list[i].val);
    delete list;
}

/*****************************************************************************/
/*****************************************************************************/
