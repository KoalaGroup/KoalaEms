/*
 * commu_station.hxx
 *
 * $ZEL: commu_station.hxx,v 2.16 2014/07/14 15:12:20 wuestner Exp $
 *
 * created before 04.03.94
 */

#ifndef _commu_station_hxx_
#define _commu_station_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <commu_list_t.hxx>
#include <commu_message.hxx>
#include <commu_io.hxx>
#include <nocopy.hxx>

using namespace std;

/*****************************************************************************/

class C_station:public nocopy
  {
  public:
    C_station();
    virtual ~C_station() {}

  private:
    string stationname;

  protected:
    typedef enum
      {
      st_non_existent, st_connecting, st_opening, st_read_ident,
      st_error_ident, st_ready, st_disconnecting, st_close
      } stat;
    int valid;
    int identifier;
    int remote;

    typedef enum
      {
      pol_none=0,
      pol_nodebug=1, // do not generate debug messages
      pol_noshow =2, // do not show log messages
      pol_nocount=4, // do not count for autoexit
      pol_nowait =8  // do not wait for this station (client) at exit
      } en_policies;
    en_policies policies;

    typedef enum
      {
      log_none, log_in, log_out, log_bi
      } logtype;

    // loglists enthalten die IDs der Clients, die an Logmessages interessiert
    // sind
    C_ints loglist_in;
    C_ints loglist_out;
    // logtemplates werden im Constructor von C_station in die loglists kopiert
    static C_ints logtempl_in;
    static C_ints logtempl_out;

    string grund;
    string extgrund;
    int err;

    static const char *io_readfehler;
    static const char *io_writefehler;
    static const char *io_readbruch;
    static const char *io_writebruch;
    static const char *io_identfehler;
    static const char *io_disconnect;
    static const char *io_verzicht;
    static const char *io_ende;
    static const char *io_openfehler;
    static const char *io_serverclose;
    virtual const char* statstr() const =0;
    void setname(const string& name) {stationname=name;}
    virtual void logmessage(const C_message*, int) =0;

  public:
    virtual string name() const {return stationname;}
    virtual stat status() const =0;
    string reason() const {return(grund);}
    int errnum() const {return(err);}
    //int hidden() const {return !regulaer;}
    //void hide() {regulaer=0;}
    int pol_show() const {return (policies&pol_noshow)==0;}
    int pol_count() const {return (policies&pol_nocount)==0;}
    int pol_nwait() const {return (policies&pol_nowait)!=0;}
    int pol_debug() const {return (policies&pol_nodebug)==0;}
    void pol_debug(int v) {policies=(en_policies)(policies|pol_nodebug);}
    en_policies get_policies() const {return policies;}
    int is_remote() const {return(remote);}
    virtual int addmessage(C_message*, int)=0;
    //virtual int addmessage(msgheader*, int*, int);
    int ident() const {return identifier;}
    int setlog(int client, logtype type);
  };

/*****************************************************************************/

class C_io_station: virtual public C_station
  {
  public:
    C_io_station();
    virtual ~C_io_station();

  private:
    stat status_value;

  protected:
    C_socket *socket;
    int far;
    int towrite;
    int toread;

    C_io reading;
    C_io writing;
    C_messagelist messagelist;
    int lostmessages;
    void _setstatus(stat status) {status_value=status;}
    virtual void work_read_non_existent()=0;
    virtual void work_write_non_existent()=0;
    virtual void work_read_connecting()=0;
    virtual void work_write_connecting()=0;
    virtual void work_read_opening()=0;
    virtual void work_write_opening()=0;
    virtual void work_read_read_ident()=0;
    virtual void work_write_read_ident()=0;
    virtual void work_read_error_ident()=0;
    virtual void work_write_error_ident()=0;
    virtual void work_read_ready()=0;
    virtual void work_write_ready()=0;
    virtual void work_read_disconnecting()=0;
    virtual void work_write_disconnecting()=0;
    virtual const char* statstr() const;

    virtual void proceed_message(C_message*)=0;

  public:
    virtual stat status() const {return(status_value);}
    int socknum() const {if (socket) return(int(*socket)); else return(-1);}
    int to_write() const {return(towrite);}
    int to_read() const {return(toread);}
    void read_sel();
    void write_sel();
    void except_sel();
    virtual int addmessage(C_message*, int);
    //virtual int addmessage(msgheader* header, int* body, int nodel)
    //  {return(C_station::addmessage(header, body, nodel));}
    int is_far() const {return(far);}
  };

/*****************************************************************************/

class C_local_station: virtual public C_station
  {
  public:
    C_local_station();
    virtual ~C_local_station();

  protected:
    virtual const char* statstr() const;

  public:
    virtual stat status() const {return(st_ready);}
  };

/*****************************************************************************/

class C_server;

class C_local_server;

class C_io_server;

class C_client;

//class C_local_client;

class C_io_client;

#endif

/*****************************************************************************/
/*****************************************************************************/
