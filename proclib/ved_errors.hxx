/*
 * proclib/ved_errors.hxx
 * 
 * created: 19.08.94 PW  
 * 
 * $ZEL: ved_errors.hxx,v 2.11 2014/07/14 15:11:54 wuestner Exp $
 */

#ifndef _ved_errors_hxx_
#define _ved_errors_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errors.hxx>
#include "proc_conf.hxx"
#include <ems_err.h>
#include "proc_ved.hxx"
#include <errorcodes.h>
#include <requesttypes.h>

using namespace std;

/*****************************************************************************/

class C_ved_error: public C_error
  {
  public:
    C_ved_error(const C_ved_error&);
    C_ved_error(const C_VED*, C_confirmation*);
    C_ved_error(const C_VED*, const C_confirmation*);
    C_ved_error(const C_VED*, C_confirmation*, ostringstream&);
    C_ved_error(const C_VED*, const C_confirmation*, ostringstream&);
    C_ved_error(const C_VED*, C_confirmation*, string);
    C_ved_error(const C_VED*, const C_confirmation*, string);
    C_ved_error(const C_VED*, int);
    C_ved_error(const C_VED*, EMSerrcode);
    C_ved_error(const C_VED*, errcode);
    C_ved_error(const C_VED*, ostringstream&);
    C_ved_error(const C_VED*, string);
    C_ved_error(const C_VED*, int, ostringstream&);
    C_ved_error(const C_VED*, int, string);
    C_ved_error(const C_VED*, EMSerrcode, ostringstream&);
    C_ved_error(const C_VED*, EMSerrcode, string);
    C_ved_error(const C_VED*, errcode, ostringstream&);
//    C_ved_error(C_VED* ved, const char* s);
    virtual ~C_ved_error();

    static const int e_ved;
    typedef
      enum{string_err, sys_err, ems_err, req_err, pl_err, rt_err} errtypes;
    typedef union
      {
      int syserr;
      EMSerrcode ems_err;
      errcode req_err;
      plerrcode pl_err;
      rterrcode rt_err;
      } errorcode;

  protected:
    void printerror(stringstream& s, int typ, ems_u32* buf, int size) const;
    class C_error_data
      {
      friend class C_ved_error;
      public:
        C_error_data();
        C_error_data(const C_error_data&);
        ~C_error_data();

      private:
        int refcount;
        C_ved_error::errtypes typ;
        C_ved_error::errorcode code;
        string init_str;
        string error_str;
        string vedname;
        C_confirmation conf;
      };
    C_error_data* errdat;
    string errstr() const;

  public:
    const C_confirmation& errconf() const {return(errdat->conf);}
    //C_VED* vedptr() const {return(errdat->ved);}
    string vedname() const;
    errtypes errtype() const {return(errdat->typ);}
    errorcode error() const {return(errdat->code);}

    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_ved);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    virtual string message() const;
  };

class C_exec_Error: public C_ved_error
  {
  public:
    C_exec_Error(const C_exec_Error&);
    C_exec_Error(const C_ved_error&);
    virtual ~C_exec_Error();
  };

class C_other_Error: public C_ved_error
  {
  public:
    C_other_Error(const C_other_Error&);
    C_other_Error(const C_ved_error&);
    virtual ~C_other_Error();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
