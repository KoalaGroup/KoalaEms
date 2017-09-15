/*
 * ved_errors.hxx
 * 
 * created: 19.08.94 PW  
 * 25.03.1998 PW: adapded for <string>
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _ved_errors_hxx_
#define _ved_errors_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <errors.hxx>
#include "proc_conf.hxx"
#include <ems_err.h>
#include "proc_ved.hxx"
#include <errorcodes.h>
#include <requesttypes.h>

/*****************************************************************************/

class C_ved_error: public C_error
  {
  public:
    C_ved_error(const C_ved_error&);
    C_ved_error(const C_VED*, C_confirmation*);
    C_ved_error(const C_VED*, const C_confirmation*);
    C_ved_error(const C_VED*, C_confirmation*, OSTRINGSTREAM&);
    C_ved_error(const C_VED*, const C_confirmation*, OSTRINGSTREAM&);
    C_ved_error(const C_VED*, C_confirmation*, STRING);
    C_ved_error(const C_VED*, const C_confirmation*, STRING);
    C_ved_error(const C_VED*, int);
    C_ved_error(const C_VED*, EMSerrcode);
    C_ved_error(const C_VED*, errcode);
    C_ved_error(const C_VED*, OSTRINGSTREAM&);
    C_ved_error(const C_VED*, STRING);
    C_ved_error(const C_VED*, int, OSTRINGSTREAM&);
    C_ved_error(const C_VED*, int, STRING);
    C_ved_error(const C_VED*, EMSerrcode, OSTRINGSTREAM&);
    C_ved_error(const C_VED*, EMSerrcode, STRING);
    C_ved_error(const C_VED*, errcode, OSTRINGSTREAM&);
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
    void printerror(STRINGSTREAM& s, int typ, ems_u32* buf, int size) const;
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
        STRING init_str;
        STRING error_str;
        STRING vedname;
        C_confirmation conf;
      };
    C_error_data* errdat;
    STRING errstr() const;

  public:
    const C_confirmation& errconf() const {return(errdat->conf);}
    //C_VED* vedptr() const {return(errdat->ved);}
    STRING vedname() const;
    errtypes errtype() const {return(errdat->typ);}
    errorcode error() const {return(errdat->code);}

    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_ved);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    virtual STRING message() const;
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
