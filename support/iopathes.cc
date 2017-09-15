/*
 * iopathes.cc
 * 
 * created: 12.03.1998 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <cerrno>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "versions.hxx"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>

#include "errors.hxx"
#include "iopathes.hxx"

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: iopathes.cc,v 1.25 2009/07/03 12:27:23 wuestner Exp $")
#define XVERSION

///////////////////////////////////////////////////////////////////////////////
C_iopath::C_iopath(const STRING& pathname, C_iopath::io_dir dir, int verbatim)
:pathname_(pathname), verbatim_(verbatim),
 path_(-1), wbuffersize_(0), wbuffer_(0), sockptr(0), oldsockptr(0),
 typ_(iotype_none), dir_(dir)
{
    open();
}
///////////////////////////////////////////////////////////////////////////////
C_iopath::C_iopath(int path, io_dir dir)
:pathname_(""), verbatim_(-1),
 path_(path), wbuffersize_(0), wbuffer_(0), sockptr(0), oldsockptr(0),
 typ_(iotype_none), dir_(dir)
{
try
  {
  filemode_=get_mode(path_);
  typ_=detect_type(path_);
  }
catch(C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  }
}
///////////////////////////////////////////////////////////////////////////////
C_iopath::C_iopath(const char* hostname, int port, C_iopath::io_dir dir)
:verbatim_(0),
 path_(-1), wbuffersize_(0), wbuffer_(0), sockptr(0), oldsockptr(0),
 typ_(iotype_none), dir_(dir)
{
OSTRINGSTREAM st;
st<<hostname<<':'<<port;
pathname_=st.str();
if ((dir_!=iodir_input) && (dir_!=iodir_output))
  {
  cerr<<"C_iopath::C_iopath: dir must be input or output"<<endl;
  return;
  }
try
  {
  path_=socket_open(hostname, port, dir_);
  filemode_=get_mode(path_);
  typ_=detect_type(path_);
  }
catch(C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  }
}
///////////////////////////////////////////////////////////////////////////////
C_iopath::C_iopath()
:pathname_(""), path_(-1), sockptr(0), oldsockptr(0), typ_(iotype_none)
{}
///////////////////////////////////////////////////////////////////////////////
C_iopath::~C_iopath()
{
if (sockptr || oldsockptr)
  {
  delete oldsockptr;
  delete sockptr;
  }
else
  if (path_>0) ::close(path_);
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::init(const STRING& pathname, C_iopath::io_dir dir, int verbatim)
{
if (typ_!=iotype_none)
  {
  throw new C_program_error("C_iopath::init called after other initialisation");
  }
pathname_=pathname;
verbatim_=verbatim;
dir_=dir;
wbuffersize_=0;
wbuffer_=0;
open();
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::open()
{
if ((dir_!=iodir_input) && (dir_!=iodir_output))
  {
  cerr<<"C_iopath::C_iopath: dir must be input or output"<<endl;
  return;
  }
try
  {
  if (verbatim_)
    {
    path_=file_open(pathname_, dir_);
    }
  else
    {
    if (pathname_=="-") // stdin or stdout
      {
      path_=(dir_==iodir_input)?0:1;
      }
#ifdef NO_ANSI_CXX
    else if (pathname_.index(":")>=0) // socket
#else
    else if (pathname_.find(':')!=string::npos) // socket
#endif
      {
      path_=socket_open(pathname_, dir_);
      }
    else
      {
      path_=file_open(pathname_, dir_);
      }
    }
  filemode_=get_mode(path_);
  typ_=detect_type(path_);
  }
catch(C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  }
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::reopen()
{
if (typ_!=iotype_none)
  {
  cerr<<"C_iopath::reopen: path not closed"<<endl;
  return;
  }
if (pathname_=="")
  {
  cerr<<"C_iopath::reopen: no pathname given"<<endl;
  return;
  }
open();
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::close()
{
if (::close(path_)<0) cerr<<"C_iopath::close: "<<strerror(errno)<<endl;
path_=-1;
if (sockptr) delete sockptr;
sockptr=0;
typ_=iotype_none;
}
///////////////////////////////////////////////////////////////////////////////
int C_iopath::file_open(const STRING& pathname, C_iopath::io_dir dir)
{
    int path;

    /* LINUX_LARGEFILE is defined as zero for every sane operating system */
    path=::open(pathname.c_str(),
            ((dir==iodir_input)?O_RDONLY:O_WRONLY|O_CREAT|O_TRUNC)|
                    LINUX_LARGEFILE, 0640);
    if (path<0) {
        OSTRINGSTREAM st;
        st<<"Open file \""<<pathname<<"\" for "
                <<((dir==iodir_input)?"input":"output");
        throw new C_unix_error(errno, st);
    }
    return path;
}
///////////////////////////////////////////////////////////////////////////////
int C_iopath::socket_open(const STRING& pathname, C_iopath::io_dir dir)
{
if (oldsockptr)
  { /* passive part of connection, listen socket already open */
  sockptr=oldsockptr->sock_accept(0);
  }
else
  { /* no listen socket open */
  int active, portnum;
  STRING pname=pathname;
  STRING host, port;
#ifdef NO_ANSI_CXX
  int colon=pname.index(":");
  active=colon!=0;
  host=pname(0, colon);
  port=pname(colon+1, pname.length());
  if (strspn(port, "0123456789")==port.length())
#else
  int colon=pname.find(':');
  active=colon!=0;
  host=pname.substr(0, colon);
  port=pname.substr(colon+1, pname.length());
  if (port.find_first_not_of("0123456789")==string::npos)
#endif
    {/* tcp/ip */
    ISTRINGSTREAM st(port);
    st>>portnum;
    if (debuglevel::verbose_level)
      {
      cerr<<"connecting "<<((dir==iodir_input)?"input":"output")<<" with ";
      if (active)
        cerr<<host<<':'<<portnum<<endl;
      else
        cerr<<"port "<<portnum<<endl;
      }
    tcp_socket* socke=new tcp_socket((dir==iodir_input)?"input":"output");
    socke->create();
    if (active)
      {
      sockptr=socke;
      socke->connect(host.c_str(), portnum);
      }
    else
      {
      oldsockptr=socke;
      socke->bind(portnum);
      socke->listen();
      sockptr=socke->sock_accept(0);
      }
    }
  else
    {/* unix domain */
    if (debuglevel::verbose_level)
      {
      cerr<<"connecting "<<((dir==iodir_input)?"input":"output")
          <<" with socket "<<port;
      if (active)
        cerr<<" (active)"<<endl;
      else
        cerr<<" (passive)"<<endl;
      }
    unix_socket* socke=new unix_socket((dir==iodir_input)?"input":"output");
    socke->create();
    if (active)
      {
      sockptr=socke;
      socke->connect(port.c_str());
      }
    else
      {
      oldsockptr=socke;
      socke->bind(port.c_str());
      socke->listen();
      sockptr=socke->sock_accept(0);
      }
    }
  }
return sockptr->path();
}
///////////////////////////////////////////////////////////////////////////////
int C_iopath::socket_open(const char* hostname, int port, C_iopath::io_dir dir)
{
if (oldsockptr)
  { /* passive part of connection, listen socket already open */
  sockptr=oldsockptr->sock_accept(0);
  }
else
  { /* no listen socket open */
  int active;
  active=hostname!=0&&hostname[0]!=0;
  if (debuglevel::verbose_level)
    {
    cerr<<"connecting "<<((dir==iodir_input)?"input":"output")<<" with ";
    if (active)
      cerr<<hostname<<':'<<port<<endl;
    else
      cerr<<"port "<<port<<endl;
    }
  tcp_socket* socke=new tcp_socket((dir==iodir_input)?"input":"output");
  socke->create();
  if (active)
    {
    sockptr=socke;
    socke->connect(hostname, port);
    }
  else
    {
    oldsockptr=socke;
    socke->bind(port);
    socke->listen();
    sockptr=socke->sock_accept(0);
    }
  }
return sockptr->path();
}
///////////////////////////////////////////////////////////////////////////////
C_iopath::io_types C_iopath::detect_type(int path)
{
if (path<0) return iotype_none;
// {
// char* s="nix";
// if (S_ISFIFO(filemode_)) s="fifo";
// else if (S_ISDIR(filemode_)) s="directory";
// else if (S_ISCHR(filemode_)) s="character special file";
// else if (S_ISBLK(filemode_)) s="block special file";
// else if (S_ISREG(filemode_)) s="regular file";
// cerr<<"file is "<<s<<endl;
// }
if (is_fifo(path)) return iotype_fifo;
if (is_socket(path)) return iotype_socket;
if (is_tape(path)) return iotype_tape;
return iotype_file;
}
///////////////////////////////////////////////////////////////////////////////
int C_iopath::is_socket(int path)
{
struct sockaddr my_address;
SOCKLEN_TYPE len;
int res;

len=sizeof(struct sockaddr);
if (getsockname(path, &my_address, &len)==0)
  { // path is a socket
  res=1;
  }
else
  {
  if (errno!=ENOTSOCK)
    {
    throw new C_unix_error(errno, "C_iopath::is_socket:");
    }
  else
    res=0;
  }
return res;
}
///////////////////////////////////////////////////////////////////////////////
mode_t C_iopath::get_mode(int path)
{
struct stat buffer;
mode_t& mode=buffer.st_mode;

if (fstat(path, &buffer)!=0)
  {
  throw new C_unix_error(errno, "C_iopath::fstat:");
  }
return mode;
}
///////////////////////////////////////////////////////////////////////////////
int C_iopath::is_fifo(int path)
{
return S_ISFIFO(filemode_);
}
///////////////////////////////////////////////////////////////////////////////
int C_iopath::is_tape(int path)
{
struct mtop mt_com;
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;
int res;
if (ioctl(path, MTIOCTOP, &mt_com)==0)
  res=1;
else
  {
  // cerr<<"is_tape: errno="<<strerror(errno)<<endl;
  res=0;
  }
return res;
}
///////////////////////////////////////////////////////////////////////////////
ostream& C_iopath::print_socket(ostream& os) const
{
struct sockaddr peer_address, my_address;
SOCKLEN_TYPE len;

len=sizeof(struct sockaddr);
if (getsockname(path_, &my_address, &len)!=0)
  os<<"(getsockname: "<<strerror(errno)<<")";
else
  os<<my_address;
os<<endl<<"  connected with ";
len=sizeof(struct sockaddr);
if (getpeername(path_, &peer_address, &len)!=0)
  os<<"(getpeername: "<<strerror(errno)<<")";
else
  os<<peer_address;
return os;
}
///////////////////////////////////////////////////////////////////////////////
ostream& C_iopath::print(ostream& os) const
{
switch (dir_)
  {
  case iodir_both: os<<"in- and output through "; break;
  case iodir_input: os<<"input from "; break;
  case iodir_output: os<<"output to "; break;
  }
switch (typ_)
  {
  case iotype_none: os<<"nothing? "; break;
  case iotype_file: os<<"plain file "; break;
  case iotype_tape: os<<"tape "; break;
  case iotype_fifo: os<<"fifo "; break;
  case iotype_socket: os<<"socket "; break;
  }
if (typ_==iotype_socket)
  {
  print_socket(os);
  }
else
  {
  if (pathname_=="")
    os<<"(descriptor "<<path_<<")";
  else
    os<<pathname_;
  }
return(os);
}
///////////////////////////////////////////////////////////////////////////////
ostream& operator <<(ostream& os, const C_iopath& rhs)
{
return(rhs.print(os));
}
///////////////////////////////////////////////////////////////////////////////
ssize_t C_iopath::xsend(void* data, size_t num)
{
// XXX
// inkonsistent: return 0?, num(rest)?, idx(gesendet)?, void+exception?
ssize_t idx=0;

while (num)
  {
  ssize_t dort=::write(path_, static_cast<char*>(data)+idx, num);
  if (dort>0)
    {
    num-=dort;
    idx+=dort;
    }
  else if (dort<=0)
    {
    if (dort==0) errno=EPIPE;
    if (errno==EAGAIN) return idx;
    if (errno!=EINTR) throw C_unix_error(errno, "C_iopath::xsend");
    }
  }
return idx;
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::xread(void* buf, size_t len)
{
ssize_t da, restlen;
char *bufptr;

restlen=len;
bufptr=static_cast<char*>(buf);
while (restlen)
  {
  da=::read(path_, bufptr, restlen);
  if (da>0)
    {
    bufptr+=da;
    restlen-=da;
    }
  else if (da==0)
    {
    throw new C_unix_error(EPIPE, "C_iopath::xread");
    }
  else
    {
    if (errno!=EINTR)
      {
      throw new C_unix_error(errno, "C_iopath::xread");
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
ssize_t C_iopath::write(void* data, size_t num, int destroy)
{
// XXX
// inkonsistent: return -1/gesendete Bytes (::write)?, 0/exception (xsend)?
ssize_t res;
switch (typ_)
  {
  case iotype_tape:
    res=::write(path_, data, num);
    break;
  case iotype_file:
  case iotype_fifo:
    res=::write(path_, data, num);
    break;
  case iotype_socket:
    res=xsend(data, num);
    break;
  case iotype_none:
  default: throw new C_program_error("illegal iotype in C_iopath::write");
  }
if (destroy) delete static_cast<char*>(data);
return res;
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::flush_buffer()
{
    if ((wbuffersize_>0) && (wbufferidx_>0)) {
        ssize_t res=write(wbuffer_, wbufferidx_, 0);
        if (res!=wbufferidx_) {
            wbufferidx_=0;
            throw new C_unix_error(errno, "C_iopath::flush_buffer write");
        }
    }
    wbufferidx_=0;
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::write_buffered(void* data, size_t num, int destroy)
{
    if ((wbufferidx_>0) && (num+wbufferidx_>wbuffersize_)) {
        int res=write(wbuffer_, wbufferidx_, 0);
        if (res<0) {
            wbufferidx_=0;
            throw new C_unix_error(errno, "C_iopath::write_buffered");
        }
        wbufferidx_=0;
    }
    if (num>=wbuffersize_) {
        int res=write(data, num, destroy);
        if (res<0) {
            throw new C_unix_error(errno, "C_iopath::write_buffered");
        }
    } else {
        //bcopy((const char*)data, wbuffer_+wbufferidx_, num);
        memcpy(wbuffer_+wbufferidx_, data, num);
        wbufferidx_+=num;
    }
}
///////////////////////////////////////////////////////////////////////////////
int C_iopath::wbuffersize(int buffersize)
{
int bsize=wbuffersize_;
if ((wbuffersize_>0) && (wbufferidx_>0))
  {
  int res=write(wbuffer_, wbufferidx_, 0);
  if (res!=wbufferidx_)
    {
    wbufferidx_=0;
    throw new C_unix_error(errno, "C_iopath::wbuffersize write");
    }
  }
delete[] wbuffer_;
wbuffersize_=buffersize;
wbuffer_=new char[wbuffersize_];
wbufferidx_=0;
return bsize;
}
///////////////////////////////////////////////////////////////////////////////
ssize_t C_iopath::read(void* data, size_t num)
{
ssize_t res;
switch (typ_)
  {
  case iotype_tape:
    res=::read(path_, data, num);
    if (res<0) cerr<<"C_iopath::read tape: "<<strerror(errno)<<endl;
    if (res==0) throw new C_status_error(C_status_error::err_filemark);
    break;
  case iotype_file:
    res=::read(path_, data, num);
    if (res==0) throw new C_status_error(C_status_error::err_end_of_file);
    break;
  case iotype_fifo:
  case iotype_socket:
    xread(data, num);
    res=num;
    break;
  case iotype_none:
  default: {
    OSTRINGSTREAM st;
    st<<"illegal iotype in C_iopath::read: "<<static_cast<int>(typ_);
    throw new C_program_error(st);
    }
  }
return res;
}
///////////////////////////////////////////////////////////////////////////////
STRING C_iopath::readln()
{
    STRING st;
    char c;
    do {
        if (::read(path_, &c, 1)!=1)
            break;
        if (c!='\n')
            st+=c;
    } while (c!='\n');
    return st;
}
///////////////////////////////////////////////////////////////////////////////
ssize_t C_iopath::write_noblock(void* data, size_t num)
{
ssize_t dort=::write(path_, data, num);
if (dort<0)
  {
  if ((errno==EAGAIN) || (errno==EINTR))
    dort=0;
  else
    throw C_unix_error(errno, "C_iopath::write_noblock");
  }
return dort;
}
///////////////////////////////////////////////////////////////////////////////
void C_iopath::noblock()
{
if (fcntl(path_, F_SETFL, O_NDELAY)==-1)
    throw new C_unix_error(errno, "C_iopath::noblock");
}
///////////////////////////////////////////////////////////////////////////////
off_t C_iopath::seek(ems_u64 offs, int whence)
{
return lseek(path_, offs, whence);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
