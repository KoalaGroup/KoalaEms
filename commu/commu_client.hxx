/*
 * commu_client.hxx
 *
 * $ZEL: commu_client.hxx,v 2.14 2014/07/14 15:12:19 wuestner Exp $
 *
 * created 29.07.94
 * 
 */

#ifndef _commu_client_hxx_
#define _commu_client_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "commu_station.hxx"
#include <ems_err.h>
#include "commu_list_t.hxx"
#include "commu_message.hxx"
#include "commu_server.hxx"

using namespace std;

/*****************************************************************************/

class C_client: virtual public C_station
  {
  public:
    C_client();
    virtual ~C_client();

  private:

  protected:
    C_server_infos servers;
    int def_unsol;
    static char staticstring[1024];

    virtual void proceed_OpenVED(C_message*);
    virtual void proceed_CloseVED(C_message*);
    virtual void proceed_GetHandle(C_message*);
    virtual void proceed_GetName(C_message*);
    virtual void proceed_SetUnsolicited(C_message*);
    virtual void proceed_SetExitCommu(C_message*);
    virtual void logmessage(const C_message*, int);

  public:
    int operator < (const C_client&) const;
    int operator > (const C_client&) const;
    void server_ok(int identifier, int res/*, int error*/);
    void server_died(int identifier);
    void bye();
    int ident() const {return(identifier);}
    const char* getservername(int) const; // nur fuer log wichtig
    void addunsolmessage(msgheader*, ems_u32*, const C_server*);
    void addunsolmessage(C_message*, const C_server*);
    void addunsolmessage(C_message*, int);
    int has_server(int) const;
    const C_server_infos* getserverlist() const {return(&servers);}
    virtual ostream& print(ostream&) const;
  };
ostream& operator <<(ostream&, const C_client&);

/*****************************************************************************/

class C_clientlist: public C_list<C_client>
  {
  public:
    C_clientlist(int, string name);
    virtual ~C_clientlist();
  private:
    int leer_;
  public:
    int leer() const {return(leer_);}
    C_io_client* findsock(int) const;
    C_client* get(char*) const;
    C_client* get(const string&) const;
    C_client* get(int) const;
    void free(int);
    void readfield(fd_set*, int&) const;
    void writefield(fd_set*, int&) const;
    void exceptfield(fd_set*, int&) const;
    virtual void counter();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
