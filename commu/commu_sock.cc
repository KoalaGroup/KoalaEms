/*
 * commu_sock.cc
 * 
 * created before 03.09.93
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
/*#include <sys/uio.h>*/ /* wegen struct iovec */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <debug.hxx>
#include <commu_sock.hxx>
#include <commu_log.hxx>
#include <ems_err.h>
#include <ems_errors.hxx>
#include <compat.h>
#include <config_types.h>
#include <versions.hxx>

extern C_log elog, nlog, dlog;

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_sock.cc,v 2.26 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_socket::~C_socket()
{
TR(C_socket::~C_socket)
if (sock>-1) ::close(sock);
}

/*****************************************************************************/

int C_socket::getstatus()
{
int res, val;
socklen_type len;

TR(C_socket::getstatus)
len=sizeof(val);
res=getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&val, &len);
if (res==0)
  return(val);
else
  return(errno);
}

/*****************************************************************************/

void C_socket::close()
{
TR(C_socket::close)
if (sock>-1) ::close(sock);
sock=-1;
}

/*****************************************************************************/

C_unix_socket::C_unix_socket()
:C_socket(), owner(0)
{
unsigned int optval=1;

TR(C_unix_socket::C_unix_socket)
memset((char*)&addr, 0, sizeof(struct sockaddr_un));
addr.sun_family=AF_UNIX;
sock=socket(AF_UNIX, SOCK_STREAM, 0);
if (sock==-1)
    throw new C_unix_error(errno, "socket(AF_UNIX, SOCK_STREAM, 0)");

//#ifndef __linux__
if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
    (char*)&optval, sizeof(optval))==-1)
  {
  elog << "C_unix_socket::C_unix_socket:setsockopt" << error << flush;
  throw new C_unix_error(errno, "setsockopt(sock, ..., SO_REUSEADDR)");
  }
//#endif
if (fcntl(sock, F_SETFL, O_NDELAY)==-1)
  {
  elog << "C_unix_socket::C_unix_socket:fcntl" << error << flush;
  throw new C_unix_error(errno, "fcntl(sock, F_SETFL, O_NDELAY)");
  }
}

/*****************************************************************************/

C_unix_socket::C_unix_socket(int s)
:C_socket()
{
TR(C_unix_socket::C_unix_socket(int))
memset((char*)&addr, 0, sizeof(struct sockaddr_un));
sock=s;
}

/*****************************************************************************/

C_unix_socket::~C_unix_socket()
{
TR(C_unix_socket::~C_unix_socket)
if (owner && (addr.sun_path[0]!=0)) unlink(addr.sun_path);
}

/*****************************************************************************/

int C_unix_socket::bind()
{
int res;
mode_t mask;

TR(C_unix_socket::bind)
DL(dlog << level(D_COMM) << "bind: " << addr << flush;)
mask=umask(0);
res=::bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
if (res==-1) return(-1);
umask(mask);
owner=1;
return(0);
}

/*****************************************************************************/

// void C_unix_socket::setname(const char *name)
// {
// TR(C_unix_socket::setname)
// strcpy(addr.sun_path, name);
// }
void C_unix_socket::setname(const string& name)
{
    strcpy(addr.sun_path, name.c_str());
}

/*****************************************************************************/

C_socket* C_unix_socket::accept()
{
int s;
struct sockaddr_un sa;
socklen_type size;

C_unix_socket *newsocket;

TR(C_unix_socket::accept)
size=sizeof(struct sockaddr_un);
s=::accept(sock, (struct sockaddr*)&sa, &size);
if (fcntl(s, F_SETFL, O_NDELAY)==-1)
  {
  elog << "C_unix_socket::accept: fcntl" << error << flush;
  throw new C_unix_error(errno, "fcntl(s, F_SETFL, O_NDELAY)");
  }
newsocket=new C_unix_socket(s);
return(newsocket);
}

/*****************************************************************************/

void C_unix_socket::connect()
{
int res;
int size;

TR(C_unix_socket::connect)
DL(dlog << level(D_COMM) << "connect: " << addr << flush;)
size=sizeof(struct sockaddr_un);
res=::connect(sock, (struct sockaddr*)&addr, size);
if ((res==0) || (errno==EINPROGRESS)) return;
throw new C_unix_error(errno, "connect unix socket");
}

/*****************************************************************************/

ostream& C_unix_socket::print(ostream& os) const
{
os << addr.sun_path;
return(os);
}

/*****************************************************************************/
/*****************************************************************************/

C_tcp_socket::C_tcp_socket()
:C_socket()
{
unsigned int optval=1;

TR(C_tcp_socket::C_tcp_socket)
memset((char*)&addr, 0, sizeof(struct sockaddr_in));
addr.sin_family=AF_INET;
sock=socket(AF_INET, SOCK_STREAM, 0);
if (sock==-1)
    throw new C_unix_error(errno, "socket(AF_INET, SOCK_STREAM, 0)");

if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
    (char*)&optval, sizeof(optval))==-1)
  {
  elog << "C_tcp_socket::C_tcp_socket:setsockopt(SO_REUSEADDR)" << error
      << flush;
  throw new C_unix_error(errno, "setsockopt(sock, SOL_SOCKET, SO_REUSEADDR)");
  }

if (fcntl(sock, F_SETFL, O_NDELAY)==-1)
  {
  elog << "C_tcp_socket::C_tcp_socket:fcntl" << error << flush;
  throw new C_unix_error(errno, "fcntl(sock, F_SETFL, O_NDELAY)");
  }

if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
    (char*)&optval, sizeof(optval))==-1)
  {
  elog << "C_tcp_socket::C_tcp_socket:setsockopt(TCP_NODELAY)" << error
      << flush;
  throw new C_unix_error(errno, "setsockopt(sock, IPPROTO_TCP, TCP_NODELAY)");
  }

return;
}

/*****************************************************************************/

C_tcp_socket::C_tcp_socket(int s)
:C_socket()
{
TR(C_tcp_socket::C_tcp_socket(int))
memset((char*)&addr, 0, sizeof(struct sockaddr_in));
sock=s;
}

/*****************************************************************************/

C_tcp_socket::~C_tcp_socket()
{
TR(C_tcp_socket::~C_tcp_socket)
}

/*****************************************************************************/

int C_tcp_socket::bind()
{
TR(C_tcp_socket::bind)
DL(dlog << level(D_COMM) << "bind: " << addr << flush;)
return(::bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)));
}

/*****************************************************************************/

// void C_tcp_socket::setname(const char *name, int port)
// {
// struct hostent *host;
// 
// TR(C_tcp_socket::setname)
// //cout<<"C_tcp_socket::setname: name="<<name<<endl;
// if (name==NULL)
//   addr.sin_addr.s_addr=htonl(INADDR_ANY);
// else
//   {
//   if ((host=gethostbyname((char*)name))==NULL)
//     {
//     //cout<<"host="<<(void*)host<<endl;
//     throw new C_ems_error(EMSE_HostUnknown, "gethostbyname()");
//     }
//   addr.sin_addr.s_addr= *(int*)(host->h_addr_list[0]);
//   //int x=addr.sin_addr.s_addr;
//   //cout<<"s_addr="<<(x&0xff)<<'.'<<((x>>8)&0xff)<<'.'<<((x>>16)&0xff)<<'.'<<
//   //    ((x>>24)&0xff)<<endl;
//   }
// addr.sin_port=htons(port);
// }
void C_tcp_socket::setname(const string& name, int port)
{
    struct hostent *host;

    if (name=="")
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
    else {
        if ((host=gethostbyname(name.c_str()))==NULL) {
            throw new C_ems_error(EMSE_HostUnknown, "gethostbyname()");
        }
    addr.sin_addr.s_addr= *(int*)(host->h_addr_list[0]);
    //int x=addr.sin_addr.s_addr;
    //cout<<"s_addr="<<(x&0xff)<<'.'<<((x>>8)&0xff)<<'.'<<((x>>16)&0xff)<<'.'<<
    //    ((x>>24)&0xff)<<endl;
    }
    addr.sin_port=htons(port);
}

/*****************************************************************************/

C_socket* C_tcp_socket::accept()
{
int s;
socklen_type size;
unsigned int optval=1;
struct sockaddr_in sa;
C_tcp_socket *newsocket;

TR(C_tcp_socket::accept)
size=sizeof(struct sockaddr_in);
s=::accept(sock, (struct sockaddr*)&sa, &size);

if (fcntl(s, F_SETFL, O_NDELAY)==-1)
  {
  elog << "C_tcp_socket::accept: fcntl" << error << flush;
  throw new C_unix_error(errno, "fcntl(s, F_SETFL, O_NDELAY)");
  }
if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
    (char*)&optval, sizeof(optval))==-1)
  {
  elog << "C_tcp_socket::accept: setsockopt" << error << flush;
  throw new C_unix_error(errno, "setsockopt(s, IPPROTO_TCP, TCP_NODELAY)");
  }
newsocket=new C_tcp_socket(s);
return(newsocket);
}

/*****************************************************************************/

void C_tcp_socket::connect()
{
int res;
unsigned int optval=1;

TR(C_tcp_socket::connect)
DL(dlog << level(D_COMM) << "connect: " << addr << flush;)

res=::connect(sock, (struct sockaddr*)&addr,
    sizeof(struct sockaddr_in));
//throw new C_unix_error(errno, "connect tcp socket: TCP_NODELAY nicht aufgerufen");
if ((res==0) || (errno==EINPROGRESS)) return;
//throw new C_unix_error(errno, "connect tcp socket");
if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
    (char*)&optval, sizeof(optval))==-1)
  {
  elog << "C_tcp_socket::connect:setsockopt(TCP_NODELAY)" << error
      << flush;
  throw new C_unix_error(errno, "setsockopt(sock, IPPROTO_TCP, TCP_NODELAY)");
  }
}

/*****************************************************************************/

void C_tcp_socket::connected()
{
unsigned int optval=1;

TR(C_tcp_socket::connected)

if (fcntl(sock, F_SETFL, O_NDELAY)==-1)
  {
  elog << "C_tcp_socket::accepted: fcntl" << error << flush;
  throw new C_unix_error(errno, "fcntl(sock, F_SETFL, O_NDELAY)");
  }
if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
    (char*)&optval, sizeof(optval))==-1)
  {
  elog << "C_tcp_socket::accepted: setsockopt" << error << flush;
  throw new C_unix_error(errno, "setsockopt(sock, IPPROTO_TCP, TCP_NODELAY)");
  }
}

/*****************************************************************************/

ostream& C_tcp_socket::print(ostream& os) const
{
struct sockaddr_in address;
socklen_type size;

size=sizeof(struct sockaddr_in);
if (sock==-1)
  os << "none";
else
  {
  if (getsockname(sock, (sockaddr*)&address, &size)==0)
    os << inet_ntoa(address.sin_addr) << ':' << ntohs(address.sin_port);
  else
    {
    os << "(no getsockname: " << strerror(errno) << ") ";
    os << inet_ntoa(addr.sin_addr) << ':' << ntohs(addr.sin_port);
    }
  }
return(os);
}

/*****************************************************************************/

ostream& operator << (ostream& os, const C_socket& rhs)
{
return(rhs.print(os));
}

/*****************************************************************************/
/*****************************************************************************/
