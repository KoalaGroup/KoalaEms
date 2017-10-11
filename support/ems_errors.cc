/*
 * support/ems_errors.cc
 * 
 * created 07.07.95 PW
 * 
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "ems_errors.hxx"
#include <conststrings.h>

#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: ems_errors.cc,v 2.10 2014/07/14 15:09:53 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

static C_ems_error_registerer registerer;

const int C_ems_error::e_ems(4);

/*****************************************************************************/

C_ems_error_registerer::C_ems_error_registerer()
{
C_error::manager.regist(C_ems_error::e_ems, C_ems_error::create);
}

/*****************************************************************************/

C_ems_error::C_ems_error(EMSerrcode code, const char* message)
:emscode(code), message_(message)
{}

/*****************************************************************************/

C_ems_error::C_ems_error(EMSerrcode code, const string& msg)
:emscode(code), message_(msg)
{}

/*****************************************************************************/

C_ems_error::C_ems_error(EMSerrcode code, ostringstream& s)
:emscode(code)
{
message_=s.str();
}

/*****************************************************************************/

C_ems_error::~C_ems_error()
{
}

/*****************************************************************************/

ostream& C_ems_error::print(ostream& os) const
{
os << message_ << ": " << EMS_errstr(emscode);
return(os);
}

/*****************************************************************************/

C_outbuf& C_ems_error::bufout(C_outbuf& ob) const
{
ob << e_ems << static_cast<int>(emscode) << message_;
return(ob);
}

/*****************************************************************************/

string C_ems_error::message(void) const
{
return(message_);
}

/*****************************************************************************/

C_error* C_ems_error::create(C_inbuf& inbuf)
{
string s;
EMSerrcode code;
code=static_cast<EMSerrcode>(inbuf.getint());
inbuf >> s;
return(new C_ems_error(code, s));
}

/*****************************************************************************/
/*****************************************************************************/
