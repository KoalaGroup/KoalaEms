/*
 * commu_server.hxx
 *
 * $ZEL: commu_server.hxx,v 2.14 2004/11/18 19:31:31 wuestner Exp $
 *
 * created 29.07.94
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _commu_server_hxx_
#define _commu_server_hxx_

#include "config.h"
#include "cxxcompat.hxx"

#include "commu_station.hxx"
#include "commu_list_t.hxx"
#include "commu_message.hxx"
#include "commu_cdb.hxx"

/*****************************************************************************/

class C_server: virtual public C_station
  {
  public:
    C_server(const char*, en_policies);
    virtual ~C_server();

  private:
  protected:
    C_ints unsollist;
    int refcount;
    int ver_ved;
    int ver_req;
    int ver_unsol;
    virtual void logmessage(const C_message*, int);

  public:
    int operator < (const C_server&) const;
    int operator > (const C_server&) const;
    virtual void addclient(C_client*);
    virtual void removeclient(C_client*);
    int setunsol(int, int);
    void getver(ems_u32*, ems_u32*, ems_u32*) const;
    virtual const C_VED_addr& get_addr() const=0;
    const C_ints* getunsollist() const {return(&unsollist);}
    virtual ostream& print(ostream&) const;
  };
ostream& operator <<(ostream&, const C_server&);
/*****************************************************************************/

class C_serverlist: public C_list<C_server>
  {
  public:
    C_serverlist(int, STRING);
    virtual ~C_serverlist();
  protected:
    int last_id;
  public:
    C_server* get(int) const;
    C_server* get(char*) const;
    C_server* get(const STRING&) const;
    C_io_server* findsock(int) const;
    void free(char*);
    void free(const STRING&);
    void readfield(fd_set*, int&) const;
    void writefield(fd_set*, int&) const;
    void exceptfield(fd_set*, int&) const;
    int nextid();
  };

/*****************************************************************************/

class C_server_info: public nocopy
  {
  public:
    C_server_info(int gl_id, int lo_id): local_id(lo_id), global_id(gl_id),
        valid(0) {}
    virtual ~C_server_info() {}

    int local_id;
    int global_id;
    int valid;
    virtual ostream& print(ostream&) const;
  };
ostream& operator <<(ostream&, const C_server_info&);
/*****************************************************************************/

class C_server_infos: public C_list<C_server_info>
  {
  public:
    C_server_infos(int s, STRING n):C_list<C_server_info>(s, n) {}
    virtual ~C_server_infos() {}
  protected:
    int last_local_id;
  public:
    int valid(int) const;              // local_id gueltig
    int global(int) const;             // local_id --> global_id
    int local(int) const;              // global_id --> local_id
    void setvalid(int handle, int val);// valid <-- val
    void free(int);
    int add(int);                   // global_id --> new local_id
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
