/*
 * ved_errors.cc
 * 
 * created: 1994-Aug-18 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <string.h>
#include <requesttypes.h>
#include <errorcodes.h>
#include <conststrings.h>
#include "ved_errors.hxx"
#include <versions.hxx>

VERSION("2007-03-31", __FILE__, __DATE__, __TIME__,
"$ZEL: ved_errors.cc,v 2.13 2007/04/05 18:39:50 wuestner Exp $")
#define XVERSION

// Auswertung einer Fehlermeldung in einer Confirmation.

/*****************************************************************************/

const int C_ved_error::e_ved(5);

/*****************************************************************************/

C_ved_error::C_error_data::C_error_data()
:refcount(1), init_str(""), error_str(""), vedname("no VED")
{}

/*****************************************************************************/

C_ved_error::C_error_data::~C_error_data()
{}

/*****************************************************************************/

C_ved_error::~C_ved_error()
{
if (--errdat->refcount==0) delete errdat;
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_ved_error& error)
:errdat(error.errdat)
{
errdat->refcount++;
}

/*****************************************************************************/

C_exec_Error::C_exec_Error(const C_exec_Error& error)
:C_ved_error(error)
{}

/*****************************************************************************/

C_exec_Error::C_exec_Error(const C_ved_error& error)
:C_ved_error(error)
{}

/*****************************************************************************/

C_exec_Error::~C_exec_Error()
{}

/*****************************************************************************/

C_other_Error::C_other_Error(const C_other_Error& error)
:C_ved_error(error)
{}

/*****************************************************************************/

C_other_Error::C_other_Error(const C_ved_error& error)
:C_ved_error(error)
{}

/*****************************************************************************/

C_other_Error::~C_other_Error()
{}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, EMSerrcode error, OSTRINGSTREAM& ss)
{
errdat=new C_error_data;
errdat->init_str=ss.str();
errdat->typ=ems_err;
errdat->code.ems_err=error;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, EMSerrcode error, STRING s)
{
errdat=new C_error_data;
errdat->init_str=s;
errdat->typ=ems_err;
errdat->code.ems_err=error;
//cerr << "C_ved_error::C_ved_error(C_VED*, EMSerrcode, String) not complete" << endl;
//errdat->ved=ved;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, C_confirmation* confi)
{
errdat=new C_error_data;
errdat->typ=req_err;
errdat->code.ems_err=(EMSerrcode)confi->buffer(0);
if (confi) errdat->conf=*confi;
delete confi;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, const C_confirmation* confi)
{
errdat=new C_error_data;
errdat->typ=req_err;
errdat->code.ems_err=(EMSerrcode)confi->buffer(0);
if (confi) errdat->conf=*confi;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, C_confirmation* confi, OSTRINGSTREAM& ss)
{
errdat=new C_error_data;
errdat->init_str=ss.str();
errdat->typ=req_err;
errdat->code.ems_err=(EMSerrcode)confi->buffer(0);
if (confi) errdat->conf=*confi;
delete confi;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

// C_ved_error::C_ved_error(C_VED* ved, const C_confirmation* confi, OSTRINGSTREAM& ss)
// {
// errdat=new C_error_data;
// char* s=ss.str();
// errdat->init_str=s;
// delete[] s;
// errdat->typ=req_err;
// errdat->code.ems_err=(EMSerrcode)confi->buffer(0);
// if (confi) errdat->conf=*confi;
// if (ved) errdat->vedname=ved->name();
// }

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, C_confirmation* confi, STRING s)
{
errdat=new C_error_data;
errdat->init_str=s;
errdat->typ=req_err;
errdat->code.ems_err=(EMSerrcode)confi->buffer(0);
if (confi) errdat->conf=*confi;
delete confi;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

// C_ved_error::C_ved_error(C_VED* ved, const C_confirmation* confi, STRING s)
// {
// errdat=new C_error_data;
// errdat->init_str=s;
// errdat->typ=req_err;
// errdat->code.ems_err=(EMSerrcode)confi->buffer(0);
// if (confi) errdat->conf=*confi;
// if (ved) errdat->vedname=ved->name();
// }

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, int error, OSTRINGSTREAM& ss)
{
errdat=new C_error_data;
errdat->init_str=ss.str();
errdat->typ=sys_err;
errdat->code.syserr=error;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, int error, STRING s)
{
errdat=new C_error_data;
errdat->init_str=s;
errdat->typ=sys_err;
errdat->code.syserr=error;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, int error)
{
errdat=new C_error_data;
errdat->typ=sys_err;
errdat->code.syserr=error;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, OSTRINGSTREAM& ss)
{
errdat=new C_error_data;
errdat->init_str=ss.str();
errdat->typ=string_err;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

C_ved_error::C_ved_error(const C_VED* ved, STRING s)
{
errdat=new C_error_data;
errdat->init_str=s;
errdat->typ=string_err;
if (ved) errdat->vedname=ved->name();
}

/*****************************************************************************/

STRING C_ved_error::vedname() const
{
return errdat->vedname;
}

/*****************************************************************************/

STRING C_ved_error::errstr() const
{
if (errdat->error_str=="")
  {
  STRINGSTREAM ss;

  switch (errdat->typ)
    {
    case string_err:
      return(errdat->init_str);
    case sys_err:
      if (errdat->init_str!="") errdat->error_str=errdat->init_str+": ";
      errdat->error_str+=strerror(errdat->code.syserr);
      break;
    case ems_err:
      if (errdat->init_str!="") errdat->error_str=errdat->init_str+": ";
      errdat->error_str+=EMS_errstr(errdat->code.ems_err);
      break;
    case req_err:
      //cerr << "C_ved_error::errstr() unvollstaendig?" << endl;
      if (errdat->conf.valid()==0) return("Error in C_ved_error: conf not valid");
      if (errdat->init_str!="") ss << errdat->init_str << ": ";
      printerror(ss, errdat->conf.request(), (ems_u32*)errdat->conf.buffer(),
          errdat->conf.size());
#ifdef NO_ANSI_CXX
      char* s;
      s=ss.str();
      errdat->error_str=s;
      delete[] s;
#else
      errdat->error_str=ss.str();
#endif
      break;
    case pl_err:
      return("Error in C_ved_error: typ==pl_err");
    case rt_err:
      return("Error in C_ved_error: typ==rt_err");
    default:
      return("Error in C_ved_error: typ ist unsinnig");
    }
  }
return(errdat->error_str);
}

/*****************************************************************************/

static void printvorspann(STRINGSTREAM &s, int level)
{
int i;
s << "  ";
for (i=0; i<level; i++) s << "--";
}

/*****************************************************************************/
static void
printplerror(STRINGSTREAM &s, ems_u32* buf, unsigned int errlen, unsigned int level)
{
    int procno, err, pos;

    procno=buf[0];
    err=buf[1];
    errlen-=3;
    pos=3;
    if (err==plErr_RecursiveCall) {
        printvorspann(s, level);
        s << "in procedure " << procno << ": " << PL_errstr((plerrcode)err)
                << endl;
        printplerror(s, buf+3, errlen, level+1);
        pos+=buf[2];
        errlen-=buf[2];
    } else {
        if (errlen!=buf[2]) {
            int extralen,i;
            extralen=errlen-buf[2];
            printvorspann(s, level-1);
            s << "(extra data:" << setiosflags(ios::hex|ios::showbase);
            for (i=0; i<extralen; i++)  s << ' ' << buf[pos+buf[2]+i];
            s << resetiosflags(ios::hex|ios::showbase) << ')' << endl;
            errlen=buf[2];
            if (!level) {
                printvorspann(s, level);
                s << "length in error message inconsistent" << endl;
            }
        }
        printvorspann(s, level);
        s << "in procedure " << procno << ": Error: "
                << PL_errstr((plerrcode)err) << endl;
    }
    if (errlen) {
        unsigned int i;
        printvorspann(s, level);
        s << "additional data:" << setiosflags(ios::hex|ios::showbase);
        for (i=0; i<errlen; i++) s << ' ' << buf[pos+i];
        s << resetiosflags(ios::hex|ios::showbase) << endl;
    }
}
/*****************************************************************************/
void C_ved_error::printerror(STRINGSTREAM& s, int typ, ems_u32* buf, int size) const
{
s << endl << R_errstr((errcode)buf[0]) << endl;
switch(buf[0])
  {
  case Err_BadProcList:
    {
    ems_u32 *weiter;
    if (typ==Req_StartProgramInvocation)
      {
      s << "in list for IS " << buf[1] << ", Trigger " << buf[2] << ':'
          << endl;
      weiter= buf+3;
      }
    else
      weiter= buf+1;
      printplerror(s, weiter, size-(weiter-buf), 0);
    }
    break;
  case Err_BufOverfl:
    if (typ==Req_StartProgramInvocation)
      s << "in list for IS " << buf[1] << ", Trigger " << buf[2] << ':'
          << endl;
    break;
  case Err_ExecProcList:
    s << " in procedure " << buf[size-2] << ": Error: "
        << PL_errstr((plerrcode)buf[size-1]) << endl;
    if (size>3)
      {
      int i;
      s << size-3 << " data words are produced before:";
      s << setiosflags(ios::hex|ios::showbase);
      for (i=1; i<size-2; i++) s << ' ' << buf[i];
      s << resetiosflags(ios::hex|ios::showbase) << endl;
    }
    break;
  case Err_TrigProc:
    s << " " << PL_errstr((plerrcode)buf[size-1]) << endl;
    if (size>2)
      {
      int i;
      s << "  additional data:";
      s << setiosflags(ios::hex|ios::showbase);
      for (i=1; i<size-1; i++) s << ' ' << buf[i];
      s << resetiosflags(ios::hex|ios::showbase) << endl;
    }
    break;
  default:
    if(size>1)
      {
      int i;
      s << "   additional data:";
      s << setiosflags(ios::hex|ios::showbase);
      for (i=1; i<size; i++) s << ' ' << buf[i];
      s << resetiosflags(ios::hex|ios::showbase) << endl;
    }
  }
//s << ends;
}

/*****************************************************************************/

ostream& C_ved_error::print(ostream& os) const
{
os << errstr();
return(os);
}

/*****************************************************************************/

C_outbuf& C_ved_error::bufout(C_outbuf& ob) const
{
ob << e_ved;
throw new C_program_error("C_ved_error::bufout");
NORETURN(ob);
}

/*****************************************************************************/

STRING C_ved_error::message(void) const
{
return("ved_error");
}

/*****************************************************************************/
/*****************************************************************************/
