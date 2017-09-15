/*
 * ems_errors.hxx
 * 
 * created 07.07.95 PW
 * 16.03.1998 PW: adapted for <string>
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _ems_errors_hxx_
#define _ems_errors_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include "errors.hxx"
#include <ems_err.h>
#include "inbuf.hxx"
#include "outbuf.hxx"

/*****************************************************************************/

class C_ems_error_registerer
  {
  public:
  C_ems_error_registerer();
  };

/*****************************************************************************/

class C_ems_error: public C_error
  {
  public:
    C_ems_error(EMSerrcode, const char*);
    C_ems_error(EMSerrcode, const STRING&);
    C_ems_error(EMSerrcode, OSTRINGSTREAM&);
    virtual ~C_ems_error();
    static const int e_ems;
  protected:
    EMSerrcode emscode;
    STRING message_;
  public:
    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_ems);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    virtual STRING message() const;
    EMSerrcode code() const {return(emscode);}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
