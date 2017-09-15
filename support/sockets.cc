/*
 * sockets.cc
 * 
 * created 25.03.95 PW
 */

#define _BSD_SOURCE

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "versions.hxx"

#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>     /* wegen iovec */
#include <socket_prot.h> /* sys/socket.h, aber geschuetzt (wegen ultrix4.4) */
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "sockets.hxx"
#include "errors.hxx"
#include "compat.h"

VERSION("2007-04-11", __FILE__, __DATE__, __TIME__,
"$ZEL: sockets.cc,v 2.24 2007/04/11 11:38:52 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

sock::sock(STRING name)
:name_(name), path_(-1)
{}

/*****************************************************************************/

sock::sock(int path, STRING name)
:name_(name), path_(path)
{}

/*****************************************************************************/
void sock::close()
{
if (path_>=0) ::close(path_);
path_=-1;
}
/*****************************************************************************/
sock::~sock()
{
close();
}
/*****************************************************************************/

void sock::listen()
{
if (::listen(path_, 8)==-1)
    throw new C_unix_error(errno, "socket listen");
}

/*****************************************************************************/

tcp_socket::tcp_socket(STRING name)
:sock(name)
{}

/*****************************************************************************/

tcp_socket::tcp_socket(int path, STRING name)
:sock(path, name)
{}

/*****************************************************************************/

tcp_socket::~tcp_socket()
{}

/*****************************************************************************/

void tcp_socket::create()
{
if ((path_=::socket(AF_INET, SOCK_STREAM, 0))==-1)
    throw new C_unix_error(errno, "open tcp socket");
}

/*****************************************************************************/

void tcp_socket::ndelay()
{
unsigned int optval=1;

if (fcntl(path_, F_SETFL, O_NDELAY)==-1)
    throw new C_unix_error(errno, "tcp socket fcntl");
if (setsockopt(path_, IPPROTO_TCP, TCP_NODELAY, &optval,
    sizeof(optval))==-1)
    throw new C_unix_error(errno, "tcp socket setsockopt");
}

/*****************************************************************************/

void tcp_socket::bind(short port)
{
struct sockaddr_in addr;

bzero(&addr, sizeof(struct sockaddr_in));
addr.sin_family=AF_INET;
addr.sin_addr.s_addr=htonl(static_cast<in_addr_t>(0x00000000)); // INADDR_ANY
addr.sin_port=htons(port);
if (::bind(path_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr_in))==-1)
  {
  OSTRINGSTREAM s;
  s << "tcp socket bind(" << port << ")";
  throw new C_unix_error(errno, s);
  }
int optval=1;
if (setsockopt(path_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0)
  {
  cerr<<"setsockopt(SO_REUSEADDR): "<<strerror(errno)<<endl;
  }
}

/*****************************************************************************/
void tcp_socket::connect(const char* name, short port)
{
    struct sockaddr_in addr;
    in_addr_t iaddr;

    if ((iaddr=inet_addr(name))==static_cast<in_addr_t>(-1)) {
        struct hostent *host;
        if ((host=gethostbyname(name))==0) {
            OSTRINGSTREAM s;
            s << "cannot resolve hostname " << name;
            throw new C_program_error(s);
        }
        iaddr=*reinterpret_cast<in_addr_t*>(host->h_addr_list[0]);
    }

    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=iaddr;
    addr.sin_port=htons(port);
    int res=::connect(path_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr_in));
    if ((res!=0) && (errno!=EINPROGRESS)) {
        OSTRINGSTREAM s;
        s << "tcp socket connect(" << name << ", " << port << ")";
        throw new C_unix_error(errno, s);
    }
}
/*****************************************************************************/

sock* tcp_socket::sock_accept(int ndelay)
{
int ns;
SOCKLEN_TYPE size;
sock* s;
struct sockaddr addr;

size=sizeof(struct sockaddr);
ns=::accept(path_, &addr, &size);
if (ns==-1)
  {
  OSTRINGSTREAM s;
  s << "tcp socket accept(" << name_ << ")";
  throw new C_unix_error(errno, s);
  }
s=new tcp_socket(ns, name()+"_accepted");
if (ndelay) s->ndelay();
return(s);
}

/*****************************************************************************/

unix_socket::unix_socket(STRING name)
:sock(name), fname("") 
{}

/*****************************************************************************/

unix_socket::unix_socket(int path, STRING name)
:sock(path, name), fname("") 
{}

/*****************************************************************************/

unix_socket::~unix_socket()
{
if (fname!="") unlink(fname.c_str());
}

/*****************************************************************************/

void unix_socket::create()
{
if ((path_=socket(AF_UNIX, SOCK_STREAM, 0))==-1)
    throw new C_unix_error(errno, "open unix socket");
}

/*****************************************************************************/

void unix_socket::ndelay()
{
if (fcntl(path_, F_SETFL, O_NDELAY)==-1)
    throw new C_unix_error(errno, "unix socket fcntl");
}

/*****************************************************************************/

void unix_socket::bind(const char* name)
{
struct sockaddr_un addr;

bzero(&addr, sizeof(struct sockaddr_un));
addr.sun_family=AF_UNIX;
strcpy(addr.sun_path, name);
mode_t mask=umask(0);
int res=::bind(path_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr_un));
umask(mask);
if (res==-1)
  {
  OSTRINGSTREAM s;
  s << "unix socket bind(" << name << ")";
  throw new C_unix_error(errno, s);
  }
else
  fname=name;
}

/*****************************************************************************/

void unix_socket::connect(const char* name)
{
struct sockaddr_un addr;

bzero(&addr, sizeof(struct sockaddr_un));
addr.sun_family=AF_UNIX;
strcpy(addr.sun_path, name);
int res=::connect(path_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr_un));
if ((res!=0)/* && (errno=!EINPROGRESS)*/)
  {
  OSTRINGSTREAM s;
  s << "unix socket connect(" << name << ")";
  throw new C_unix_error(errno, s);
  }
}

/*****************************************************************************/

sock* unix_socket::sock_accept(int ndelay)
{
int ns;
SOCKLEN_TYPE size;
sock* s;
struct sockaddr addr;

size=sizeof(struct sockaddr);
ns=::accept(path_, &addr, &size);
if (ns==-1)
  {
  OSTRINGSTREAM s;
  s << "unix socket accept(" << name_ << ")";
  throw new C_unix_error(errno, s);
  }
s=new unix_socket(ns, name()+"_accepted");
if (ndelay) s->ndelay();
return(s);
}

/*****************************************************************************/
ostream& operator <<(ostream& os, const struct sockaddr& addr)
{
switch (addr.sa_family)
  {
#ifdef AF_APPLETALK
  case AF_APPLETALK: os<<"APPLETALK"; break;
#endif
#ifdef AF_AAL5
  case AF_AAL5     : os<<"AAL5"; break;
#endif
#ifdef AF_BRIDGE
  case AF_BRIDGE   : os<<"BRIDGE"; break;
#endif
#ifdef AF_CCITT
  case AF_CCITT    : os<<"CCITT"; break;
#endif
#ifdef AF_CHAOS
  case AF_CHAOS    : os<<"CHAOS"; break;
#endif
#ifdef AF_CTF
  case AF_CTF      : os<<"CTF"; break;
#endif
#ifdef AF_DATAKIT
  case AF_DATAKIT  : os<<"DATAKIT"; break;
#endif
#ifdef AF_DECnet
  case AF_DECnet   : os<<"DECnet"; break;
#endif
#ifdef AF_DLI
  case AF_DLI      : os<<"DLI"; break;
#endif
#ifdef AF_ECMA
  case AF_ECMA     : os<<"ECMA"; break;
#endif
#ifdef AF_HYLINK
  case AF_HYLINK   : os<<"HYLINK"; break;
#endif
#ifdef AF_IMPLINK
  case AF_IMPLINK  : os<<"IMPLINK"; break;
#endif
  case AF_INET     : os<<"INET";
    {
    const sockaddr_in* a=reinterpret_cast<const struct sockaddr_in*>(&addr);
    os<<": "<<inet_ntoa(a->sin_addr)<<':'<<ntohs(a->sin_port);
    }
    break;
#ifdef AF_INET6
  case AF_INET6    : os<<"INET6"; break;
#endif
#ifdef AF_IPX
  case AF_IPX      : os<<"IPX"; break;
#endif
#ifdef AF_ISO
  case AF_ISO      : os<<"ISO/OSI"; break;
//case AF_OSI ==AF_ISO
#endif
#ifdef AF_LAT
  case AF_LAT      : os<<"LAT"; break;
#endif
#ifdef AF_LINK
  case AF_LINK     : os<<"LINK"; break;
#endif
#ifdef AF_NETMAN
  case AF_NETMAN   : os<<"NETMAN"; break;
#endif
#ifdef AF_NETROM
  case AF_NETROM   : os<<"NETROM"; break;
#endif
#ifdef AF_NS
  case AF_NS       : os<<"NS"; break;
#endif
#ifdef AF_PUP
  case AF_PUP      : os<<"PUP"; break;
#endif
#ifdef AF_ROUTE
  case AF_ROUTE    : os<<"ROUTE"; break;
#endif
#ifdef AF_SNA
  case AF_SNA      : os<<"SNA"; break;
#endif
#ifdef AF_UNSPEC
  case AF_UNSPEC   : os<<"UNSPEC"; break;
#endif
  case AF_UNIX     : os<<"UNIX";
    {
    const sockaddr_un* a=reinterpret_cast<const struct sockaddr_un*>(&addr);
    os<<": "<<a->sun_path;
    }
    break;
#ifdef AF_USER
  case AF_USER     : os<<"USER"; break;
#endif
#ifdef AF_WAN
  case AF_WAN      : os<<"WAN"; break;
#endif
#ifdef AF_X25
  case AF_X25      : os<<"X25"; break;
#endif
#ifdef _pseudo_AF_XTP
  case _pseudo_AF_XTP:os<<"XTP"; break;
#endif
#ifdef pseudo_AF_XTP
  case pseudo_AF_XTP:os<<"XTP"; break;
#endif
  default          : os<<"family "<<addr.sa_family; break;
  }
return os;
}
/*****************************************************************************/
/*****************************************************************************/
