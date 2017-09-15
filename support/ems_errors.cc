/*
 * ems_errors.cc
 * 
 * created 07.07.95 PW
 * 16.03.1998 PW: adapded for <string>
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "ems_errors.hxx"
#include <conststrings.h>

#include <versions.hxx>

VERSION("Mar 16 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: ems_errors.cc,v 2.9 2006/11/27 10:33:35 wuestner Exp $")
#define XVERSION

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

C_ems_error::C_ems_error(EMSerrcode code, const STRING& msg)
:emscode(code), message_(msg)
{}

/*****************************************************************************/

C_ems_error::C_ems_error(EMSerrcode code, OSTRINGSTREAM& s)
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

STRING C_ems_error::message(void) const
{
return(message_);
}

/*****************************************************************************/

C_error* C_ems_error::create(C_inbuf& inbuf)
{
STRING s;
EMSerrcode code;
code=static_cast<EMSerrcode>(inbuf.getint());
inbuf >> s;
return(new C_ems_error(code, s));
}

/*****************************************************************************/
/*****************************************************************************/
