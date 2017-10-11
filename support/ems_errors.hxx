/*
 * support/ems_errors.hxx
 * 
 * created 07.07.95 PW
 * 
 * $ZEL: ems_errors.hxx,v 2.10 2014/07/14 15:09:53 wuestner Exp $
 */

#ifndef _ems_errors_hxx_
#define _ems_errors_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "errors.hxx"
#include <ems_err.h>
#include "inbuf.hxx"
#include "outbuf.hxx"

using namespace std;

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
    C_ems_error(EMSerrcode, const string&);
    C_ems_error(EMSerrcode, ostringstream&);
    virtual ~C_ems_error();
    static const int e_ems;
  protected:
    EMSerrcode emscode;
    string message_;
  public:
    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_ems);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    virtual string message() const;
    EMSerrcode code() const {return(emscode);}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
