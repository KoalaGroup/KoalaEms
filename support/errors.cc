/*
 * errors.cc
 * 
 * created 27.01.95 PW
 * 16.03.1998 PW: adapted for <string>
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 * 18.06.1998 PW: errno changed to error
 * 14.07.1998 PW: C_errorbox deleted
 * 13.01.1999 PW: C_error::instance_counter introduced
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "versions.hxx"

#include "errors.hxx"

VERSION("Jan 13 1999", __FILE__, __DATE__, __TIME__,
"$ZEL: errors.cc,v 2.18 2007/10/23 11:18:06 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_error::C_manager C_error::manager;

static C_error_registerer registerer;

const int C_error::e_unknown(0);
const int C_error::e_none(1);
const int C_error::e_system(2);
const int C_error::e_program(3);
//const int C_ems_error::e_ems(4);
//const int C_ved_error::e_ved(5);
const int C_error::e_status(6);

/*****************************************************************************/

C_error::C_manager::C_manager()
:num(0), list(0)
{}

/*****************************************************************************/

C_error::C_manager::~C_manager()
{}

/*****************************************************************************/

void C_error::C_manager::regist(int typ, C_error* (*creator)(C_inbuf&))
{
int i;
entry* help;

help=new entry[num+1];
for (i=0; i<num; i++) help[i]=list[i];
delete[] list;
list=help;
list[num].typ=typ;
list[num].func=creator;
num++;
}

/*****************************************************************************/

C_error* C_error::C_manager::create(C_inbuf& ib)
{
int typ;
C_error* (*creator)(C_inbuf&);
int i;

typ=ib.getint();
for (i=0; (i<num) && (list[i].typ!=typ); i++);
if (i==num)
  {
    OSTRINGSTREAM s;
  s << "C_error::C_manager::create(C_inbuf&): unknown type " << typ;
  throw new C_program_error(s);
  }
creator=list[i].func;
return(creator(ib));
}

/*****************************************************************************/
/*****************************************************************************/

C_error_registerer::C_error_registerer()
{
C_error::manager.regist(C_error::e_unknown, C_error::create);
C_error::manager.regist(C_error::e_none, C_none_error::create);
C_error::manager.regist(C_error::e_system, C_unix_error::create);
C_error::manager.regist(C_error::e_program, C_program_error::create);
C_error::manager.regist(C_error::e_status, C_status_error::create);
}

/*****************************************************************************/
/*****************************************************************************/

int C_error::instance_counter=0;

C_error::C_error()
{
deleted=0;
if (instance_counter!=0)
  {
  cerr<<"C_error: counter="<<instance_counter<<endl;
  }
instance_counter++;
}

C_error::~C_error()
{
if (deleted)
  {
  cerr<<"C_error:: double free"<<endl;
  sleep(1);
  *static_cast<int*>(0)=17;
  }
deleted=1;
instance_counter--;
if (instance_counter!=0)
  {
  cerr<<"~C_error: counter="<<instance_counter<<endl;
  }
}

/*****************************************************************************/

C_outbuf& C_error::bufout(C_outbuf& ob) const
{
ob << e_unknown;
return(ob);
}

/*****************************************************************************/

C_error* C_error::create(C_inbuf&)
{
throw new C_program_error("C_error::create(C_inbuf&): type not valid");
NORETURN(0);
}

/*****************************************************************************/

C_error* C_error::extract(C_inbuf& inbuf)
{
return(C_error::manager.create(inbuf));
}

/*****************************************************************************/
/*****************************************************************************/

C_none_error::C_none_error()
{}

C_none_error::~C_none_error()
{}

/*****************************************************************************/

STRING C_none_error::message(void) const
{
return("none");
}

/*****************************************************************************/

ostream& C_none_error::print(ostream& os) const
{
os << "no error";
return(os);
}

/*****************************************************************************/

C_outbuf& C_none_error::bufout(C_outbuf& ob) const
{
ob << e_none;
return(ob);
}

/*****************************************************************************/

C_error* C_none_error::create(C_inbuf&)
{
return(new C_none_error);
}

/*****************************************************************************/
/*****************************************************************************/

C_unix_error::C_unix_error(int error, const char* message)
:error_(error), message_(message)
{}

/*****************************************************************************/

C_unix_error::C_unix_error(int error, const STRING& msg)
:error_(error), message_(msg)
{}

/*****************************************************************************/

C_unix_error::C_unix_error(int error, OSTRINGSTREAM& s)
:error_(error)
{
//cerr<<"C_unix_error("<<error<<", "<<s.str()<<")"<<endl;
message_=s.str();
}

/*****************************************************************************/

C_unix_error::~C_unix_error()
{}

/*****************************************************************************/

STRING C_unix_error::message(void) const
{
return(message_);
}

/*****************************************************************************/

ostream& C_unix_error::print(ostream& os) const
{
os << message_ << ": " << strerror(error_);
return(os);
}

/*****************************************************************************/

C_outbuf& C_unix_error::bufout(C_outbuf& ob) const
{
ob << e_system << error_ << message_;
return(ob);
}

/*****************************************************************************/

C_error* C_unix_error::create(C_inbuf& inbuf)
{
STRING s;
int error;
inbuf >> error >> s;
return(new C_unix_error(error, s));
}

/*****************************************************************************/
/*****************************************************************************/

C_program_error::C_program_error(const char* message)
:message_(message)
{}

/*****************************************************************************/

C_program_error::C_program_error(const STRING& msg)
:message_(msg)
{}

/*****************************************************************************/

C_program_error::C_program_error(OSTRINGSTREAM& s)
{
message_=s.str();
}

/*****************************************************************************/

C_program_error::~C_program_error()
{}

/*****************************************************************************/

STRING C_program_error::message(void) const
{
return(message_);
}

/*****************************************************************************/

ostream& C_program_error::print(ostream& os) const
{
os << message_;
return(os);
}

/*****************************************************************************/

C_outbuf& C_program_error::bufout(C_outbuf& ob) const
{
ob << e_program << message_;
return(ob);
}

/*****************************************************************************/

C_error* C_program_error::create(C_inbuf& inbuf)
{
STRING s;
inbuf >> s;
return(new C_program_error(s));
}

/*****************************************************************************/
/*****************************************************************************/

ostream& operator <<(ostream& os, const C_error& rhs)
{
return(rhs.print(os));
}

/*****************************************************************************/

C_outbuf& operator <<(C_outbuf& ob, const C_error& error)
{
return(error.bufout(ob));
}
/*****************************************************************************/
C_status_error::C_status_error(status_codes code)
:code_(code)
{}
/*****************************************************************************/
C_status_error::~C_status_error()
{}
/*****************************************************************************/
C_error* C_status_error::create(C_inbuf& inbuf)
{
int code_;
inbuf>>code_;
return(new C_status_error(status_codes(code_)));
}
/*****************************************************************************/
ostream& C_status_error::print(ostream& os) const
{
switch (code_)
  {
  case err_none:
    os<<"status_error: none";
    break;
  case err_end_of_file:
    os<<"end of file";
    break;
  case err_filemark:
    os<<"filemark";
    break;
  }
os<<"";
return(os);
}
/*****************************************************************************/
STRING C_status_error::message(void) const
{
switch (code_)
  {
  case err_none:
    return "status_error: none";
    break;
  case err_end_of_file:
    return "end of file";
    break;
  case err_filemark:
    return "filemark";
    break;
  }
return "program error in C_status_error::message";
}
/*****************************************************************************/
C_outbuf& C_status_error::bufout(C_outbuf& ob) const
{
ob<<e_status<<static_cast<int>(code_);
return(ob);
}
/*****************************************************************************/
/*****************************************************************************/
