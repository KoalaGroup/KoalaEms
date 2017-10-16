/*
 * vtainer.cc
 * 
 * created 2014-Nov-04 PW
 */

#include "config.h"
#include "vtainer.hxx"
#include <versions.hxx>
#include "strawtest_data.hxx"

VERSION("2014-11-03", __FILE__, __DATE__, __TIME__,
"$ZEL$")
#define XVERSION

//vtainer<int>::initcap=256;

template<class T>
vtainer<T>::vtainer(void)
{
    data=0;
    capacity=0;
    number=0;
}

template<class T>
vtainer<T>::~vtainer()
{
    delete data;
}

template<class T>
void vtainer<T>::clear(void)
{
    number=0;
    // leave capacity and data unchanged, we can probably reuse the space
}

template<class T>
void vtainer<T>::append(T& entry)
{
    data[0]=entry;
}


template class vtainer<rawdata>;
