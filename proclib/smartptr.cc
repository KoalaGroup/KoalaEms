/*
 * proclib/smartptr.cc
 * 
 * created: 15.06.97
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include "smartptr_t.hxx"
#include <proc_namelist.hxx>
#include <caplib.hxx>
#include <proc_plist.hxx>
#include <errors.hxx>
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: smartptr.cc,v 2.9 2014/07/14 15:11:54 wuestner Exp $")

using namespace std;

/*****************************************************************************/

const int C_ptr_error::e_ptr(6);

//C_ptr_error::C_ptr_error(const C_ptr_error& error)
//{}


C_ptr_error::C_ptr_error(ostringstream& ss)
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

string C_ptr_error::message(void) const
{
return(message_);
}

/*****************************************************************************/
/*****************************************************************************/
