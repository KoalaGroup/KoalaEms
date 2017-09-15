/******************************************************************************
*                                                                             *
* proc_memberlist.cc                                                          *
*                                                                             *
* created: 08.06.97                                                           *
* last changed: 08.06.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include <stdlib.h>
#include <proc_memberlist.hxx>
#include <proc_conf.hxx>
#include <errors.hxx>
#include "versions.hxx"

VERSION("Jun 05 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_memberlist.cc,v 2.4 2004/11/26 14:44:31 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_memberlist::C_memberlist(int n)
:max(n), size_(0)
{
addresses=new int[n];
if (addresses==0) throw new C_unix_error(errno, "alloc memberlist");
}

/*****************************************************************************/

C_memberlist::C_memberlist(const C_confirmation* conf)
{
C_inbuf ib(conf->buffer(), conf->size());
ib++;
size_=max=ib.getint();
addresses=new int[max];
if (addresses==0) throw new C_unix_error(errno, "alloc memberlist");
for (int i=0; i<max; i++) addresses[i]=ib.getint();
}

/*****************************************************************************/

C_memberlist::~C_memberlist(void)
{
delete[] addresses;
}

/*****************************************************************************/

void C_memberlist::add(int addr)
{
if (size_>=max) throw new C_program_error("size of C_memberlist too small");
addresses[size_]=addr;
size_++;
}

/*****************************************************************************/
/*****************************************************************************/
