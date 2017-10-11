/*
 * proclib/proc_ioaddr.hxx
 * 
 * created: 09.06.97
 *
 * $ZEL: proc_ioaddr.hxx,v 2.10 2014/09/07 00:43:39 wuestner Exp $
 */

#ifndef _proc_ioaddr_hxx_
#define _proc_ioaddr_hxx_

#include "config.h"
#include "objecttypes.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <outbuf.hxx>
#include <proc_iotype.hxx>

using namespace std;

/*****************************************************************************/

class C_io_addr
  {
  friend class C_data_io;
  friend ostream& operator <<(ostream&, const C_io_addr&);
  friend C_outbuf& operator <<(C_outbuf&, const C_io_addr&);
  public:
    C_io_addr(io_direction);
    C_io_addr(io_direction, bool lists);
    virtual ~C_io_addr();
    static C_io_addr* create(io_direction, C_inbuf&, bool lists);
  protected:
    io_direction direction;
    int argnum;
    int* args;
    bool forcelists_;
    bool uselists() const;
    bool forcelists(bool);
    virtual C_outbuf& out(C_outbuf&) const=0;
    virtual ostream& print(ostream&) const=0;
  public:
    virtual IOAddr addrtype() const=0;
    void pruefen() const;
  };

ostream& operator <<(ostream&, const C_io_addr&);

class C_io_addr_raw: public C_io_addr
  {
  public:
    C_io_addr_raw(io_direction, C_inbuf&, bool);
    C_io_addr_raw(io_direction, int);
    virtual ~C_io_addr_raw();
  protected:
    int addr;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Raw;}
  };

class C_io_addr_modul: public C_io_addr
  {
  public:
    C_io_addr_modul(io_direction, C_inbuf&, bool);
    C_io_addr_modul(io_direction, const string&, int);// (dataout upload)
    C_io_addr_modul(io_direction, const string&);
    virtual ~C_io_addr_modul();
  protected:
    string name;
    int addr;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Modul;}
    };

class C_io_addr_driver: public C_io_addr // nur als datain
  {
  public:
    C_io_addr_driver(io_direction, C_inbuf&, bool);
    C_io_addr_driver(io_direction, const string&, int, int, int);
    virtual ~C_io_addr_driver();
  protected:
    string name;
    int addr_space, offset, option;
    //virtual C_outbuf& out(C_outbuf&) const;
    //virtual ostream& print(ostream&) const;
  };

class C_io_addr_driver_mapped: public C_io_addr_driver // nur als datain
  {
  public:
    C_io_addr_driver_mapped(io_direction, C_inbuf&, bool);
    C_io_addr_driver_mapped(io_direction, const string&, int, int, int);
    virtual ~C_io_addr_driver_mapped();
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Driver_mapped;}
  };

class C_io_addr_driver_mixed: public C_io_addr_driver // nur als datain
  {
  public:
    C_io_addr_driver_mixed(io_direction, C_inbuf&, bool);
    C_io_addr_driver_mixed(io_direction, const string&, int, int, int);
    virtual ~C_io_addr_driver_mixed();
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Driver_mixed;}
  };

class C_io_addr_driver_syscall: public C_io_addr_driver // nur als datain
  {
  public:
    C_io_addr_driver_syscall(io_direction, C_inbuf&, bool);
    C_io_addr_driver_syscall(io_direction, const string&, int, int, int);
    virtual ~C_io_addr_driver_syscall();
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Driver_syscall;}
  };

class C_io_addr_socket: public C_io_addr
  {
  public:
    C_io_addr_socket(io_direction, C_inbuf&, bool);
    C_io_addr_socket(io_direction, int, int);           // dataout
    C_io_addr_socket(io_direction, const string&, int); // dataout
    C_io_addr_socket(io_direction, const char*, int);   // dataout
    C_io_addr_socket(io_direction, int);                // datain
    virtual ~C_io_addr_socket();
  protected:
    int host, port;
    string hostname;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
    unsigned int get_iaddr(const char*);
  public:
    IOAddr addrtype() const {return Addr_Socket;}
  };

class C_io_addr_v6socket: public C_io_addr
  {
  public:
    C_io_addr_v6socket(io_direction, C_inbuf&, bool);
    C_io_addr_v6socket(io_direction, ip_flags, const string&, const string&);
    C_io_addr_v6socket(io_direction, ip_flags, const char*, const char*);
    C_io_addr_v6socket(io_direction, ip_flags, const char*);
    virtual ~C_io_addr_v6socket();
  protected:
    string node, service;
    ip_flags flags;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_V6Socket;}
  };

class C_io_addr_autosocket: public C_io_addr
  {
  public:
    C_io_addr_autosocket(io_direction, C_inbuf&, bool);
    C_io_addr_autosocket(io_direction, int);
    virtual ~C_io_addr_autosocket();
  protected:
    int port;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Autosocket;}
  };

class C_io_addr_localsocket: public C_io_addr
  {
  public:
    C_io_addr_localsocket(io_direction, C_inbuf&, bool);
    C_io_addr_localsocket(io_direction, const string&);
    virtual ~C_io_addr_localsocket();
  protected:
    string name;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_LocalSocket;}
  };

class C_io_addr_file: public C_io_addr
  {
  public:
    C_io_addr_file(io_direction, C_inbuf&, bool);
    C_io_addr_file(io_direction, const string&);
    virtual ~C_io_addr_file();
  protected:
    string name;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_File;}
  };

class C_io_addr_asynchfile: public C_io_addr
  {
  public:
    C_io_addr_asynchfile(io_direction, C_inbuf&, bool);
    C_io_addr_asynchfile(io_direction, const string&);
    virtual ~C_io_addr_asynchfile();
  protected:
    string name;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_AsynchFile;}
  };

class C_io_addr_tape: public C_io_addr // nur dataout
  {
  public:
    C_io_addr_tape(io_direction, C_inbuf&, bool);
    C_io_addr_tape(io_direction, const string&);
    virtual ~C_io_addr_tape();
  protected:
    string name;
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Tape;}
  };

class C_io_addr_null: public C_io_addr // nur dataout
  {
  public:
    C_io_addr_null(io_direction, C_inbuf&, bool);
    C_io_addr_null(io_direction);        // dataout
    virtual ~C_io_addr_null();
  protected:
    virtual C_outbuf& out(C_outbuf&) const;
    virtual ostream& print(ostream&) const;
  public:
    IOAddr addrtype() const {return Addr_Null;}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
