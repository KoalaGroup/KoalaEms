/*
 * proc_namelist.cc
 * 
 * created: 08.06.97
 * last changed: 08.06.97
 */

#include <errno.h>
#include <stdlib.h>
#include <errorcodes.h>
#include "proc_namelist.hxx"
#include "proc_conf.hxx"
#include <versions.hxx>

VERSION("Jun 12 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_namelist.cc,v 2.5 2005/02/22 17:44:33 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
C_namelist::C_namelist(C_confirmation* conf)
    :size_(0), list_(0)
{
    if (conf->buffer(0)!=OK)
        return;
    size_=conf->buffer(1);
    list_=new int[size_];
    for (int i=0; i<size_; i++)
        list_[i]=(Object)conf->buffer(2+i);
}
/*****************************************************************************/
C_namelist::~C_namelist()
{
    if (list_)
        delete[] list_;
}
/*****************************************************************************/
int C_namelist::exists(int id) const
{
    for (int i=0; i<size_; i++)
        if (list_[i]==id) return 1;
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
