/*
 * commu_io_server.hxx
 * 
 * $ZEL: commu_io_server.hxx,v 2.6 2004/11/18 19:31:14 wuestner Exp $
 * 
 * created 29.07.94 PW
 */

#ifndef _commu_io_server_hxx_
#define _commu_io_server_hxx_

#include "commu_station.hxx"
#include "commu_server.hxx"
#include "commu_cdb.hxx"

/*****************************************************************************/

class C_io_server: public C_io_station, public C_server
  {
  public:
    C_io_server(const char*, en_policies);
    virtual ~C_io_server();

  private:
    C_VED_addr* addr;
    void open(C_VED_addr*);
    virtual void proceed_message_internal(C_message*);
    virtual void proceed_message_unsol(C_message*);
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

    virtual void proceed_Open(C_message*);
    virtual void proceed_Initiate(C_message*);
    virtual void proceed_Close(C_message*);

  public:
    virtual void removeclient(C_client*);
    virtual const C_VED_addr& get_addr() const {return(*addr);}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
