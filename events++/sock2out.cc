/******************************************************************************
*                                                                             *
* sock2out.cc                                                                 *
*                                                                             *
* created: 22.07.1997                                                         *
* last changed: 06.12.1997                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#define _XOPEN_SOURCE 500
#define _OSF_SOURCE

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <readargs.hxx>
#include <inbuf.hxx>
#include "swap_int.h"
#include <sockets.hxx>
#include <errors.hxx>
#include "versions.hxx"

VERSION("Jun 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: sock2out.cc,v 1.7 2005/02/11 15:45:12 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
int swap_out;
int recsize;
int inf;
typedef enum {iotype_file, iotype_tape, iotype_socket} io_types;
io_types i_type;
sock *insocket;

/*****************************************************************************/

void printstreams(ostream& os)
{
os<<endl;
os<<"input may be: "<<endl;
os<<"  - for stdin ;"<<endl;
os<<"  <filename> for regular file or tape;"<<endl;
os<<"  :<port#> or <host>:<port#> for tcp socket;"<<endl;
os<<"  :<name> for unix socket;"<<endl;
}

/*****************************************************************************/

int readargs()
{
args->helpinset(5, printstreams);
args->addoption("infile", 0, "-", "input file", "input");

args->addoption("recsize", "recsize", 65536/4, "maximum record size (tape only)",
    "recsize");
args->addoption("swap", "swap", false, "swap output", "swap");

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
recsize=args->getintopt("recsize");
swap_out=args->getboolopt("swap");
return(0);
}

/*****************************************************************************/
/*
ostream& operator <<(ostream& os, const struct sockaddr& addr)
{
switch (addr.sa_family)
  {
  case AF_UNSPEC   : os<<"UNSPEC"; break;
  case AF_UNIX     : os<<"UNIX";
    {
    sockaddr_un* a=(struct sockaddr_un*)&addr;
    os<<": "<<a->sun_path;
    }
    break;
  case AF_INET     : os<<"INET";
    {
    sockaddr_in* a=(struct sockaddr_in*)&addr;
    os<<": "<<inet_ntoa(a->sin_addr)<<':'<<ntohs(a->sin_port);
    }
    break;
  case AF_IMPLINK  : os<<"IMPLINK"; break;
  case AF_PUP      : os<<"PUP"; break;
  case AF_CHAOS    : os<<"CHAOS"; break;
  case AF_NS       : os<<"NS"; break;
  case AF_ISO      : os<<"ISO/OSI"; break;
  //case AF_OSI ==AF_ISO
  case AF_ECMA     : os<<"ECMA"; break;
  case AF_DATAKIT  : os<<"DATAKIT"; break;
  case AF_CCITT    : os<<"CCITT"; break;
  case AF_SNA      : os<<"SNA"; break;
  case AF_DECnet   : os<<"DECnet"; break;
  case AF_DLI      : os<<"DLI"; break;
  case AF_LAT      : os<<"LAT"; break;
  case AF_HYLINK   : os<<"HYLINK"; break;
  case AF_APPLETALK: os<<"APPLETALK"; break;
  case AF_ROUTE    : os<<"ROUTE"; break;
  case AF_LINK     : os<<"LINK"; break;
  case
#ifdef _XOPEN_SOURCE_EXTENDED
        _pseudo_AF_XTP
#else
        pseudo_AF_XTP
#endif
                   : os<<"AF_XTP"; break;
  case AF_NETMAN   : os<<"NETMAN"; break;
  case AF_X25      : os<<"X25"; break;
  case AF_CTF      : os<<"CTF"; break;
  case AF_WAN      : os<<"WAN"; break;
  case AF_USER     : os<<"USER"; break;
  default          : os<<"family "<<addr.sa_family; break;
  }
return os;
}
*/
/*****************************************************************************/
int do_sock_open(const char* direction, STRING& fname, int& path,
    sock*& sockptr)
{
int active, portnum;
STRING host, port;
#ifdef NO_ANSI_CXX
int colon=fname.index(":");
active=colon!=0;
host=fname(0, colon);
port=fname(colon+1, fname.length());
size_t nums=strspn(port, "0123456789");
#else
int colon=fname.find(':');
active=colon!=0;
host=fname.substr(0, colon);
port=fname.substr(colon+1, string::npos);
size_t nums=port.find_first_not_of("0123456789");
#endif
try
  {
  if (nums==port.length())
    {
    ISSTREAM st(port.c_str());
    st>>portnum;
    tcp_socket* socke=new tcp_socket(direction);
    socke->create();
    if (active)
      {
      socke->connect(host.c_str(), portnum);
      sockptr=socke;
      }
    else
      {
      socke->bind(portnum);
      socke->listen();
      sockptr=socke->sock_accept(0);
      }
    }
  else
    {
    unix_socket* socke=new unix_socket(direction);
    socke->create();
    if (active)
      {
      socke->connect(port.c_str());
      sockptr=socke;
      }
    else
      {
      socke->bind(port.c_str());
      socke->listen();
      sockptr=socke->sock_accept(0);
      }
    }
  path=sockptr->path();
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  return -1;
  }
return 0;
}
/*****************************************************************************/
int do_file_open(int dir, const char* direction, STRING& fname, int& path,
    io_types& iotype)
{
struct mtop mt_com;
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;
if (dir==0)
  path=open(fname.c_str(), O_RDONLY, 0);
else
  path=open(fname.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0640);
if (path<0)
  {
  cerr<<"open "<<direction<<" ("<<fname<<"): "<<strerror(errno)<<endl;
  return -1;
  }
if (ioctl(path, MTIOCTOP, &mt_com)==0)
  {
  iotype=iotype_tape;
  }
else
  {
  iotype=iotype_file;
  }
return 0;
}
/*****************************************************************************/
int do_stdio_open(const char* direction, int& path, io_types& iotype)
{
struct mtop mt_com;
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;
struct sockaddr my_address, peer_address;
socklen_t len=sizeof(struct sockaddr);
if (getsockname(path, &my_address, &len)==0)
  { // path is a socket
  iotype=iotype_socket;
  }
else
  {
  if (errno==ENOTSOCK)
    { // path is not a socket
    if (ioctl(path, MTIOCTOP, &mt_com)==0)
      { // path is a tape
      iotype=iotype_tape;
      }
    else
      { // path is a pipe or regular file
      iotype=iotype_file;
      }
    }
  else
    { // it is a socket, but we know nothing about it
    iotype=iotype_socket; // may be bad (?)
    }
  }
return 0;
}
/*****************************************************************************/
int do_open()
{
insocket=0;

// determine type of stream
if (infile=="-")
  { // stdin
  inf=0;                               // assume that stdin is already open ;-)
  do_stdio_open("input", inf, i_type); // But is it a simple pipe or a socket?
  }
#ifdef NO_ANSI_CXX
else if (infile.index(":")>=0)
#else
else if (infile.find(':')!=string::npos)
#endif
  { // socket
  i_type=iotype_socket;
  if (do_sock_open("input", infile, inf, insocket)<0) return -1;
  }
else
  { // regular file, fifo or tape; tape will be detected later
  if (do_file_open(0, "input", infile, inf, i_type)<0) return -1;
  }

return 0;
}
/*****************************************************************************/

int do_close()
{
if (inf>0) close(inf);
return 0;
}

/*****************************************************************************/
int xwrite(int path, int* buf, int len)
{
int da, restlen;
char *bufptr;

restlen=len*sizeof(int);
bufptr=(char*)buf;
while (restlen)
  {
  da=write(path, bufptr, restlen);
  if (da>0)
    {
    bufptr+=da;
    restlen-=da;
    }
  else
    {
    if (errno!=EINTR)
      {
      cerr<<"xwrite: "<<strerror(errno)<<endl;
      return -1;
      }
    }
  }
return 0;
}
/*****************************************************************************/
int xrecv(int path, int* buf, int len)
{
static char rest[4];
static int nrest=0;

int da, l=len*4;
char *bufptr=(char*)buf;

if (nrest)
  {
  for (int i=0; i<nrest; i++) *bufptr++=rest[i];
  l=-nrest;
  }
da=read(path, bufptr, l);
if (da<=0)
  {
  if (da<0)
    cerr<<"xrecv: "<<strerror(errno)<<endl;
  else
    cerr<<"xrecv: broken pipe or filemark"<<endl;
  return 0;
  }
da+=nrest;
nrest=da&3;
if (nrest) cerr<<"nrest="<<nrest<<endl;
if (nrest) for (int i=0; i<nrest; i++) rest[i]=bufptr[da&~3+i];
return da/4;
}
/*****************************************************************************/
void do_convert()
{
int *buf, size, res;

buf=new int[recsize];

do
  {
  size=xrecv(inf, buf, recsize);
  if (swap_out)
    {
    for (int i=0; i<size; i++) buf[i]=swap_int(buf[i]);
    }
  res=xwrite(1, buf, size);
  }
while (res==0);
}

/*****************************************************************************/

main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);
  if (do_open()<0) {do_close(); return 1;}
  do_convert();
  do_close();
  return 0;
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  return -1;
  }
}

/*****************************************************************************/
/*****************************************************************************/
