/*
 * smartptr.cc
 * 
 * created: 15.06.97
 * 25.03.1998 PW: adapded for <string>
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <stdlib.h>
#include "smartptr_t.hxx"
#include <proc_namelist.hxx>
#include <caplib.hxx>
#include <proc_plist.hxx>
#include <errors.hxx>
#include <versions.hxx>

VERSION("Jun 12 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: smartptr.cc,v 2.8 2004/11/26 14:44:36 wuestner Exp $")

volatile int xx;
/*****************************************************************************/

const int C_ptr_error::e_ptr(6);

//C_ptr_error::C_ptr_error(const C_ptr_error& error)
//{}


C_ptr_error::C_ptr_error(OSTRINGSTREAM& ss)
{
message_=ss.str();
}

C_ptr_error::~C_ptr_error()
{}

/*****************************************************************************/

ostream& C_ptr_error::print(ostream& os) const
{
os << message_;
//os << "null pointer dereferenced";
return(os);
}

/*****************************************************************************/

C_outbuf& C_ptr_error::bufout(C_outbuf& ob) const
{
ob << e_ptr;
return(ob);
}

/*****************************************************************************/

STRING C_ptr_error::message(void) const
{
return(message_);
}

/*****************************************************************************/
/*****************************************************************************/
