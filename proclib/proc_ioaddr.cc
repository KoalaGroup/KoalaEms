/*
 * proclib/proc_ioaddr.cc
 * 
 * created: 09.06.97 PW
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "proc_ioaddr.hxx"
#include "proc_conf.hxx"
#include <errors.hxx>
#include "compat.h"
#include <versions.hxx>

VERSION("2014-09-07", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_ioaddr.cc,v 2.18 2016/05/10 16:24:46 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_io_addr::C_io_addr(io_direction direction)
:direction(direction), argnum(0), args(0), forcelists_(false)
{}

/*****************************************************************************/

C_io_addr::C_io_addr(io_direction direction, bool lists)
:direction(direction), argnum(0), args(0), forcelists_(lists)
{}

/*****************************************************************************/

C_io_addr::~C_io_addr()
{
delete[] args;
}

/*****************************************************************************/

void C_io_addr::pruefen() const
{
cerr << "C_io_addr:" << endl;
cerr << "  direction=" << static_cast<int>(direction) << endl;
cerr << "  forcelists_=" << forcelists_ << "; uselists()=" << uselists() << endl; 
}

/*****************************************************************************/

bool C_io_addr::forcelists(bool f)
{
int x=forcelists_;
forcelists_=f;
return x;
}

/*****************************************************************************/

C_io_addr* C_io_addr::create(io_direction direction, C_inbuf& ib, bool lists)
{
    IOAddr addrtype;

    // if lists!=0 the new protocol with lists is in use.
    // in this case is:
    //   first word: IOAddr
    //   second word: number of arguments after IOAddr
    //   ... arguments
    //
    // if lists==0 the old protocol is in use.
    // in this case is:
    //   first word: IOAddr
    // the number of arguments after IOAddr is fixed for each addr type
    //

    addrtype=static_cast<IOAddr>(ib.getint());

switch (addrtype)
  {
  case Addr_Raw:
    return new C_io_addr_raw(direction, ib, lists);
  case Addr_Modul:
    return new C_io_addr_modul(direction, ib, lists);
  case Addr_Driver_mapped:
    return new C_io_addr_driver_mapped(direction, ib, lists);
  case Addr_Driver_mixed:
    return new C_io_addr_driver_mixed(direction, ib, lists);
  case Addr_Driver_syscall:
    return new C_io_addr_driver_syscall(direction, ib, lists);
  case Addr_Socket:
    return new C_io_addr_socket(direction, ib, lists);
  case Addr_V6Socket:
    return new C_io_addr_v6socket(direction, ib, lists);
  case Addr_Autosocket:
    return new C_io_addr_autosocket(direction, ib, lists);
  case Addr_LocalSocket:
    return new C_io_addr_localsocket(direction, ib, lists);
  case Addr_File:
    return new C_io_addr_file(direction, ib, lists);
  case Addr_AsynchFile:
    return new C_io_addr_asynchfile(direction, ib, lists);
  case Addr_Tape:
    return new C_io_addr_tape(direction, ib, lists);
  case Addr_Null:
    return new C_io_addr_null(direction, ib, lists);
  default:
    {
    ostringstream s;
    s << "C_io_addr::create(): IOAddr " << addrtype << " is not known";
    throw new C_program_error(s);
    }
  }
}

/*****************************************************************************/

bool C_io_addr::uselists() const
{
return forcelists_;
}

/*****************************************************************************/
C_io_addr_raw::C_io_addr_raw(io_direction direction, C_inbuf& ib, bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;

    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    addr=ib.getint();
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/

C_io_addr_raw::C_io_addr_raw(io_direction direction, int addr)
:C_io_addr(direction), addr(addr)
{}

/*****************************************************************************/

C_io_addr_raw::~C_io_addr_raw()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_raw::out(C_outbuf& b) const
{
b << Addr_Raw;
if (uselists())
  b<<space_for_counter()<<addr<<set_counter();
else
  b << addr;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_raw::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "raw " << hex << setiosflags(ios::showbase) << addr;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/
C_io_addr_modul::C_io_addr_modul(io_direction direction, C_inbuf& ib,
        bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;

    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }

    ib >> name;

    if (ib.empty()) // shared memory
        addr=0;
    else
        ib >> addr;
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/

C_io_addr_modul::C_io_addr_modul(io_direction direction, const string& name,
    int addr)
:C_io_addr(direction), name(name), addr(addr)
{}

/*****************************************************************************/

C_io_addr_modul::C_io_addr_modul(io_direction direction, const string& name)
:C_io_addr(direction), name(name), addr(0)
{}

/*****************************************************************************/

C_io_addr_modul::~C_io_addr_modul()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_modul::out(C_outbuf& b) const
{
b << Addr_Modul;
if (uselists())
  b<<space_for_counter()<<name<<set_counter();
else
  b << name;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_modul::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "modul " << name;
if (direction==io_out) ss << ' ' << hex << setiosflags(ios::showbase) << addr;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/
C_io_addr_socket::C_io_addr_socket(io_direction direction, C_inbuf& ib,
    bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    int v;
    ib >> v;
    if (ib.empty() || (ib.index()-here==nargs)) { // kein Host
        host=INADDR_ANY;
        port=v;
    } else {
        host=v;
        ib >> port;
    }
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/

C_io_addr_socket::C_io_addr_socket(io_direction direction, int host, int port)
:C_io_addr(direction), host(host), port(port)
{}

/*****************************************************************************/

unsigned int C_io_addr_socket::get_iaddr(const char* hostname)
{
unsigned int iaddr;
iaddr=inet_addr(const_cast<char*>(hostname));
if (iaddr==(unsigned int)INADDR_NONE)
  {
  struct hostent *ent;
  ent=gethostbyname(const_cast<char*>(hostname));
  if (ent!=0)
    iaddr=*reinterpret_cast<unsigned int*>(ent->h_addr_list[0]);
  else
    {
    ostringstream ss;
#ifdef __osf__
    switch (h_errno)
      {
      case HOST_NOT_FOUND:
        ss << "host \"" << host << "\" not found.";
        break;
      case NO_ADDRESS:
        ss << "host \"" << host << "\" has no address.";
        break;
      case NO_RECOVERY:
        ss << "nonrecoverable error in gethostbyname(" << host << ")";
        break;
      case TRY_AGAIN:
        ss << "soft error during gethostbyname(" << host
            << "); try again later.";
        break;
      default:
        ss << "unknown error " << h_errno << " in gethostbyname(" << host
            << ")";
        break;
      }
#else
    ss << "host \"" << host << "\" not found.";
#endif
    throw new C_program_error(ss);
    }
  }
return iaddr;
}

/*****************************************************************************/

C_io_addr_socket::C_io_addr_socket(io_direction direction, 
    const char* hostname, int port)
:C_io_addr(direction), port(port)
{
host=ntohl(get_iaddr(hostname));
}

/*****************************************************************************/

C_io_addr_socket::C_io_addr_socket(io_direction direction, 
    const string& hostname, int port)
:C_io_addr(direction), port(port)
{
host=ntohl(get_iaddr(hostname.c_str()));
}

/*****************************************************************************/

C_io_addr_socket::C_io_addr_socket(io_direction direction, int port)
:C_io_addr(direction), host(INADDR_ANY), port(port)
{}

/*****************************************************************************/

C_io_addr_socket::~C_io_addr_socket()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_socket::out(C_outbuf& b) const
{
b << Addr_Socket;
if (uselists())
  {
  if (host!=INADDR_ANY)
    b<<space_for_counter()<<host<<port<<set_counter();
  else
    b<<space_for_counter()<<port<<set_counter();
  }
else
  {
  if (host!=INADDR_ANY)
    b << host << port;
  else
    b << port;
  }
return b;
}

/*****************************************************************************/

ostream& C_io_addr_socket::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "socket ";
if (host!=INADDR_ANY)
    ss << ((host>>24)&0xff) << '.' << ((host>>16)&0xff) << '.'
      << ((host>>8)&0xff) << '.' << (host&0xff) << ' ';
ss << port;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/
/*****************************************************************************/
C_io_addr_v6socket::C_io_addr_v6socket(io_direction direction,
        ip_flags flags, char const *node, char const *service)
:C_io_addr(direction), node(node), service(service),
        flags(static_cast<ip_flags>(flags&~ip_passive))
{}
/*****************************************************************************/
C_io_addr_v6socket::C_io_addr_v6socket(io_direction direction,
        ip_flags flags, char const *service)
:C_io_addr(direction), node(""), service(service),
        flags(static_cast<ip_flags>(flags|ip_passive))
{}
/*****************************************************************************/
C_io_addr_v6socket::C_io_addr_v6socket(io_direction direction, C_inbuf& ib,
        bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    u_int32_t flags_;
    ib >> flags_;
    ib >> node;
    if (ib.empty() || (ib.index()-here==nargs)) { // node not given
        service=node;
        node="";
        flags=static_cast<ip_flags>(flags_|ip_passive);
    } else {
        ib >> service;
        flags=static_cast<ip_flags>(flags_&~ip_passive);
    }
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/
C_outbuf&
C_io_addr_v6socket::out(C_outbuf& b) const
{
    u_int32_t flags_=static_cast<u_int32_t>(flags);

    b << Addr_V6Socket;
    if (uselists())
        b<<space_for_counter();

    b<<flags_;
    if (flags&ip_passive) {
        b<<service;
    } else {
        b<<node;
        b<<service;
    }

    if (uselists())
        b<<set_counter();

    return b;
}
/*****************************************************************************/
ostream&
C_io_addr_v6socket::print(std::ostream& ss) const
{
    if (uselists())
        ss<<"{";
    ss << "socket ";
    if (!(flags&ip_passive))
        ss << node << ' ';
    ss << service;
    for (int i=0; i<argnum; i++)
        ss<<' '<<args[i];
    if (uselists())
        ss << '}';
    return ss;
}
/*****************************************************************************/
C_io_addr_v6socket::~C_io_addr_v6socket()
{}
/*****************************************************************************/
/*****************************************************************************/
C_io_addr_autosocket::C_io_addr_autosocket(io_direction direction, C_inbuf& ib,
    bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    ib >> port;
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/
C_io_addr_autosocket::C_io_addr_autosocket(io_direction direction, int port)
:C_io_addr(direction), port(port)
{}
/*****************************************************************************/
C_io_addr_autosocket::~C_io_addr_autosocket()
{}
/*****************************************************************************/
C_outbuf& C_io_addr_autosocket::out(C_outbuf& b) const
{
    b << Addr_Autosocket;
    if (uselists()) {
        b<<space_for_counter()<<port<<set_counter();
    } else {
        b << port;
    }
    return b;
}
/*****************************************************************************/
ostream& C_io_addr_autosocket::print(ostream& ss) const
{
    if (uselists()) ss<<"{";
    ss << "autosocket ";
    ss << port;
    for (int i=0; i<argnum; i++)
        ss<<' '<<args[i];
    if (uselists())
        ss << '}';
    return ss;
}
/*****************************************************************************/
C_io_addr_localsocket::C_io_addr_localsocket(io_direction direction,
    C_inbuf& ib, bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    ib >> name;
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/

C_io_addr_localsocket::C_io_addr_localsocket(io_direction direction,
    const string& name)
:C_io_addr(direction), name(name)
{}

/*****************************************************************************/

C_io_addr_localsocket::~C_io_addr_localsocket()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_localsocket::out(C_outbuf& b) const
{
b << Addr_LocalSocket;
if (uselists())
  b<<space_for_counter()<<name<<set_counter();
else
  b << name;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_localsocket::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "localsocket " << name;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/
C_io_addr_file::C_io_addr_file(io_direction direction, C_inbuf& ib, bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    ib >> name;
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/
C_io_addr_file::C_io_addr_file(io_direction direction, const string& name)
:C_io_addr(direction), name(name)
{}
/*****************************************************************************/
C_io_addr_file::~C_io_addr_file()
{}
/*****************************************************************************/
ostream& C_io_addr_file::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "file " << name;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}
/*****************************************************************************/
C_outbuf& C_io_addr_file::out(C_outbuf& b) const
{
b << Addr_File;
if (uselists())
  b<<space_for_counter()<<name<<set_counter();
else
  b << name;
return b;
}
/*****************************************************************************/
C_io_addr_asynchfile::C_io_addr_asynchfile(io_direction direction, C_inbuf& ib,
        bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    ib >> name;
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/
C_io_addr_asynchfile::C_io_addr_asynchfile(io_direction direction,
        const string& name)
:C_io_addr(direction), name(name)
{}
/*****************************************************************************/
C_io_addr_asynchfile::~C_io_addr_asynchfile()
{}
/*****************************************************************************/
ostream& C_io_addr_asynchfile::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "asynchfile " << name;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}
/*****************************************************************************/
C_outbuf& C_io_addr_asynchfile::out(C_outbuf& b) const
{
b << Addr_AsynchFile;
if (uselists())
  b<<space_for_counter()<<name<<set_counter();
else
  b << name;
return b;
}
/*****************************************************************************/
C_io_addr_tape::C_io_addr_tape(io_direction direction, C_inbuf& ib, bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    ib >> name;
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/

C_io_addr_tape::C_io_addr_tape(io_direction direction, const string& name)
:C_io_addr(direction), name(name)
{}

/*****************************************************************************/

C_io_addr_tape::~C_io_addr_tape()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_tape::out(C_outbuf& b) const
{
b << Addr_Tape;
if (uselists())
  b<<space_for_counter()<<name<<set_counter();
else
  b << name;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_tape::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "tape " << name;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/

C_io_addr_null::C_io_addr_null(io_direction direction, C_inbuf& ib, bool lists)
:C_io_addr(direction, lists)
{
if (lists)
  {
  argnum=ib.getint();
  if (argnum)
    {
    args=new int[argnum];
    for (int i=0; i<argnum; i++) args[i]=ib.getint();
    }
  }
}

/*****************************************************************************/

C_io_addr_null::C_io_addr_null(io_direction direction)
:C_io_addr(direction)
{}

/*****************************************************************************/

C_io_addr_null::~C_io_addr_null()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_null::out(C_outbuf& b) const
{
b << Addr_Null;
if (uselists()) b << 0;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_null::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "null";
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/
C_io_addr_driver::C_io_addr_driver(io_direction direction, C_inbuf& ib,
    bool lists)
:C_io_addr(direction, lists)
{
    int here=0, nargs=0;
    if (lists) {
        nargs=ib.getint();
        here=ib.index();
    }
    ib >> name >> addr_space >> offset >> option;
    if (lists) {
        argnum=nargs-(ib.index()-here);
        if (argnum) {
            args=new int[argnum];
            for (int i=0; i<argnum; i++) args[i]=ib.getint();
        }
    }
}
/*****************************************************************************/

C_io_addr_driver::C_io_addr_driver(io_direction direction, const string& name,
    int space, int offset, int option)
:C_io_addr(direction), name(name), addr_space(space), offset(offset),
    option(option)
{}

/*****************************************************************************/

C_io_addr_driver::~C_io_addr_driver()
{}

/*****************************************************************************/

C_io_addr_driver_mapped::C_io_addr_driver_mapped(io_direction direction,
    C_inbuf& ib, bool lists)
:C_io_addr_driver(direction, ib, lists)
{}

/*****************************************************************************/

C_io_addr_driver_mapped::C_io_addr_driver_mapped(io_direction direction,
    const string& name, int space, int offset, int option)
:C_io_addr_driver(direction, name, space, offset, option)
{}

/*****************************************************************************/

C_io_addr_driver_mapped::~C_io_addr_driver_mapped()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_driver_mapped::out(C_outbuf& b) const
{
b << Addr_Driver_mapped;
if (uselists())
  b<<space_for_counter()<<name<<addr_space<<offset<<option<<set_counter();
else
  b << name << addr_space << offset << option;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_driver_mapped::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "driver_mapped " << name << ' ' << addr_space << ' '
    << hex << setiosflags(ios::showbase) << offset << ' '
    << dec << resetiosflags(ios::showbase) << option;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/

C_io_addr_driver_mixed::C_io_addr_driver_mixed(io_direction direction,
    C_inbuf& ib, bool lists)
:C_io_addr_driver(direction, ib, lists)
{}

/*****************************************************************************/

C_io_addr_driver_mixed::C_io_addr_driver_mixed(io_direction direction,
    const string& name, int space, int offset, int option)
:C_io_addr_driver(direction, name, space, offset, option)
{}

/*****************************************************************************/

C_io_addr_driver_mixed::~C_io_addr_driver_mixed()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_driver_mixed::out(C_outbuf& b) const
{
b << Addr_Driver_mixed;
if (uselists())
  b<<space_for_counter()<<name<<addr_space<<offset<<option<<set_counter();
else
  b << name << addr_space << offset << option;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_driver_mixed::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "driver_mixed " << name << ' ' << addr_space << ' '
    << hex << setiosflags(ios::showbase) << offset << ' '
    << dec << resetiosflags(ios::showbase) << option;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/

C_io_addr_driver_syscall::C_io_addr_driver_syscall(io_direction direction,
    C_inbuf& ib, bool lists)
:C_io_addr_driver(direction, ib, lists)
{}

/*****************************************************************************/

C_io_addr_driver_syscall::C_io_addr_driver_syscall(io_direction direction,
    const string& name, int space, int offset, int option)
:C_io_addr_driver(direction, name, space, offset, option)
{}

/*****************************************************************************/

C_io_addr_driver_syscall::~C_io_addr_driver_syscall()
{}

/*****************************************************************************/

C_outbuf& C_io_addr_driver_syscall::out(C_outbuf& b) const
{
b << Addr_Driver_syscall;
if (uselists())
  b<<space_for_counter()<<name<<addr_space<<offset<<option<<set_counter();
else
  b << name << addr_space << offset << option;
return b;
}

/*****************************************************************************/

ostream& C_io_addr_driver_syscall::print(ostream& ss) const
{
if (uselists()) ss<<"{";
ss << "driver_syscall " << name << ' ' << addr_space << ' '
    << hex << setiosflags(ios::showbase) << offset << ' '
    << dec << resetiosflags(ios::showbase) << option;
for (int i=0; i<argnum; i++) ss<<' '<<args[i];
if (uselists()) ss << '}';
return ss;
}

/*****************************************************************************/

C_outbuf& operator <<(C_outbuf& b, const C_io_addr& addr)
{
return addr.out(b);
}

/*****************************************************************************/

ostream& operator <<(ostream& st, const C_io_addr& addr)
{
return addr.print(st);
}

/*****************************************************************************/
/*****************************************************************************/

