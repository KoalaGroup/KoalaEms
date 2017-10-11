/*
 * commu_io_client.hxx
 * 
 * $ZEL: commu_io_client.hxx,v 2.10 2014/07/14 15:12:19 wuestner Exp $
 * 
 * created 29.07.94
 * 
 */

#ifndef _commu_io_client_hxx_
#define _commu_io_client_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "commu_station.hxx"
#include "commu_client.hxx"
#include <ems_err.h>

using namespace std;

/*****************************************************************************/

class C_io_client: public C_io_station, public C_client
  {
  public:
    C_io_client(C_socket*);
    virtual ~C_io_client() {}
  private:
    string hostname;
    int inetaddr;
    int pid, ppid;
    string user;
    int uid, gid;
    string experiment;
    int bigendian;

    virtual void proceed_message_internal(C_message*);
//    virtual void proceed_message_unsol(C_message);
    virtual void proceed_message_ems(C_message*);
    virtual void proceed_message(C_message*);

    virtual void work_read_non_existent();
    virtual void work_write_non_existent();
    virtual void work_read_connecting();
    virtual void work_write_connecting();
    virtual void work_read_opening();
    virtual void work_write_opening();
    virtual void work_read_read_ident();
    virtual void work_write_read_ident();
    virtual void work_read_error_ident();
    virtual void work_write_error_ident();
    virtual void work_read_ready();
    virtual void work_write_ready();
    virtual void work_read_disconnecting();
    virtual void work_write_disconnecting();

    virtual void proceed_reject(C_message*, EMSerrcode);
//    void proceed_Open(C_message*);
    void proceed_ident(C_message*);
    void proceed_Close(C_message*);

  public:
    void get_io_info(const char**, int*, const char**, int*, int*,
        const char**, int*) const;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
