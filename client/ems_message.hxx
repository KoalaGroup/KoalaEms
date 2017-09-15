/*
 * ems_message.hxx
 * 
 * created: 16.03.95
 * 25.03.1998 PW: adapded for <string>
 * 05.02.1999 PW: adapted for STD_STRICT_ANSI
 */

// $ZEL: ems_message.hxx,v 2.8 2003/09/30 16:33:21 wuestner Exp $

#ifndef ems_message_hxx
#define ems_message_hxx

#include "config.h"
#include "cxxcompat.hxx"
#include <msg.h>
#include <proc_conf.hxx>
#include <proc_is.hxx>
#include <nocopy.hxx>
#include <inbuf.hxx>

/*****************************************************************************/

class C_VED_versions: public nocopy
  {
  public:
//    C_VED_versions(C_VED* commu_i);
    C_VED_versions(C_instr_system* is_i);
    ~C_VED_versions();
  protected:
//    C_VED* commu_i;
    C_instr_system* is_i;
    struct vedver
      {
      STRING name;
      int reqver, unsolver;
      vedver* next;
      };
    vedver* versroot;
    vedver* verstop;
    int getver(const STRING& ved);
  public:
    int getversions(const STRING& ved, int& reqver, int& unsolver);
    //int reqver(const STRING& ved);
    //int unsolver(const STRING& ved);
    Request request(Request, int ver) const;
    UnsolMsg unsol(UnsolMsg, int ver) const;
  };

extern C_VED_versions* ved_versions;

/*****************************************************************************/

class C_EMS_message
  {
  public:
    C_EMS_message(const C_confirmation*, int loop, int);
    C_EMS_message(const C_EMS_message&, int);
    C_EMS_message(int* conf, msgheader head, int loop, int);
    virtual ~C_EMS_message();
  protected:
    C_confirmation conf;
    int extr;
    void rest(C_inbuf&);
    int reqver, unsolver;
    STRING server, client;
    int loop;
  public:
    C_EMS_message& operator = (const C_EMS_message&);
    virtual void print();
    int* buffer() const {return(conf.buffer());}
    int size() const {return(conf.size());}
    void setver(int req, int unsol) {reqver=req; unsolver=unsol;}
    void setnames(STRING c, STRING s) {server=s; client=c;}
  };

/*****************************************************************************/

class C_int_message: public C_EMS_message
  {
  public:
    C_int_message(const C_confirmation*, int loop);
    C_int_message(const C_EMS_message&);
    C_int_message(const C_int_message&);
    virtual ~C_int_message();
  protected:
  public:
    C_int_message& operator = (const C_int_message&);
    virtual void print();
  };

/*****************************************************************************/

class C_unsol_message: public C_EMS_message
  {
  public:
    C_unsol_message(const C_confirmation*, int loop);
    C_unsol_message(const C_EMS_message&);
    C_unsol_message(const C_unsol_message&);
    virtual ~C_unsol_message();
  protected:
    C_inbuf buf;
  public:
    C_unsol_message& operator = (const C_unsol_message&);
    virtual void print();
  };

/*****************************************************************************/

class C_ems_message: public C_EMS_message
  {
  public:
    C_ems_message(const C_confirmation*, int loop);
    C_ems_message(const C_EMS_message&);
    C_ems_message(const C_ems_message&);
    virtual ~C_ems_message();
  protected:
  public:
    C_ems_message& operator = (const C_ems_message&);
    virtual void print();
  };

/*****************************************************************************/

#endif

/*****************************************************************************/
/*****************************************************************************/
