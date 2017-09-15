/*
 * commu_local_server.hxx.m4
 * 
 * $ZEL: commu_local_server.hxx.m4,v 2.14 2004/11/18 19:31:17 wuestner Exp $
 * 
 * created 30.07.94 PW
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _commu_local_server_hxx_
#define _commu_local_server_hxx_

#include "config.h"
#include "cxxcompat.hxx"

#include <commu_station.hxx>
#include <commu_server.hxx>
#include <errorcodes.h>
#include <outbuf.hxx>

/*****************************************************************************/

class C_local_dummy
  {
  public:
    C_local_dummy(){};
    virtual ~C_local_dummy(){}

  protected:
    const ems_u32* ptr_; int num_;
    define(`version', `')
    define(`Req',`virtual errcode $1(const ems_u32* ptr, int num)
        {cerr << "dummy: $1" << endl; ptr_=ptr; num_=num; return(Err_NotImpl);}')
    include(COMMON/requesttypes.inc)
  };

/*****************************************************************************/

class C_local_server: public C_local_station, public C_server,
    public C_local_dummy
  {
  public:
    C_local_server(const char*, en_policies);
    virtual ~C_local_server() {}

    struct procprop
      {
      int varlength;   /* 0: constant, 1: variable */
      int maxlength;
      const char* syntax;
      const char* text;
      };

  protected:
    C_outbuf outbuf;
    C_client *client;
    int client_id;

//  typedef errcode (C_local_server::*reqfunc)(const ems_u32* ptr, int num);
//  static reqfunc DoRequest[];

    virtual void do_message_internal(C_message*);
    virtual void do_message_ems(C_message*);

  public:
    virtual void removeclient(C_client*);
    virtual int addmessage(C_message*, int);
    // virtual int addmessage(msgheader*, int*, int)
    virtual const C_VED_addr& get_addr() const =0;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
