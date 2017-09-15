/*
 * errors.hxx
 * 
 * created 27.01.95 PW
 */

#ifndef _errors_hxx_
#define _errors_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include "outbuf.hxx"
#include "inbuf.hxx"

/*****************************************************************************/

// #ifdef __GNUC__
// #define E_UNKNOWN 0
// #define E_NONE 1
// #define E_SYSTEM 2
// #define E_PROGRAM 3
// #define E_EMS 4
// #define E_VED 5
// #define E_STATUS 6
// #endif

class C_error;

class C_error_registerer
  {
  public:
    C_error_registerer();
  };

class C_error
  {
  public:
    C_error();
    virtual ~C_error();

    static const int e_unknown, e_none, e_system, e_program, e_status;
    class C_manager
      {
      public:
        C_manager();
        ~C_manager();
      protected:
        int num;
        struct entry
          {
          int typ;
          C_error* (*func)(C_inbuf&);
          };
        entry* list;
      public:
        void regist(int typ, C_error* (*)(C_inbuf&));
        C_error* create(C_inbuf&);
      };
    friend class C_manager;
    static C_manager manager;
  public:
    static C_error* create(C_inbuf&);
    static C_error* extract(C_inbuf&);
    virtual int type() const {return(e_unknown);}
    virtual ostream& print(ostream&) const=0;
    virtual C_outbuf& bufout(C_outbuf&) const;
    virtual STRING message() const=0;
  private:
    static int instance_counter;
    int deleted;
  };

ostream& operator <<(ostream&, const C_error&);
C_outbuf& operator <<(C_outbuf&, const C_error&);

class C_unix_error: public C_error
  {
  public:
    C_unix_error(int, const char*);
    C_unix_error(int, const STRING&);
    C_unix_error(int, OSTRINGSTREAM&);
    C_unix_error(C_inbuf&);
    virtual ~C_unix_error();
  protected:
    int error_;
    STRING message_;
  public:
    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_system);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    int error() const {return(error_);}
    virtual STRING message() const;
  };

class C_program_error: public C_error
  {
  public:
    C_program_error(const char*);
    C_program_error(const STRING&);
    C_program_error(OSTRINGSTREAM&);
    C_program_error(C_inbuf&);
    virtual ~C_program_error();
  protected:
    STRING message_;
  public:
    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_program);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    virtual STRING message() const;
  };

class C_none_error: public C_error
  {
  public:
    C_none_error();
    C_none_error(C_inbuf&);
    virtual ~C_none_error();
  public:
    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_none);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    virtual STRING message() const;
  };

class C_status_error: public C_error
  {
  public:
    typedef enum {err_none, err_end_of_file, err_filemark} status_codes;
    C_status_error(status_codes);
    C_status_error(C_inbuf&);
    virtual ~C_status_error();
  protected:
    status_codes code_;
  public:
    static C_error* create(C_inbuf&);
    virtual int type() const {return(e_status);}
    virtual ostream& print(ostream&) const;
    virtual C_outbuf& bufout(C_outbuf&) const;
    int code() const {return(code_);}
    virtual STRING message() const;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
