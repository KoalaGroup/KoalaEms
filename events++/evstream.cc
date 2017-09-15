/******************************************************************************
*                                                                             *
* evstream.cc                                                                 *
*                                                                             *
* created: 07.12.97                                                           *
* last changed: 07.12.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#define _XOPEN_SOURCE 500
#define _OSF_SOURCE

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <readargs.hxx>
#include <events.hxx>
#include "versions.hxx"

VERSION("Jul 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: evstream.cc,v 1.6 2005/02/11 15:45:07 wuestner Exp $")
#define XVERSION

C_readargs* args;
int recsize, inf, hash;
STRING infile;
typedef enum {iotype_file, iotype_tape, iotype_socket} io_types;
io_types i_type;
C_evinput* evinput;

/*****************************************************************************/
int readargs()
{
args->addoption("hash", "hash", 0, "hash", "hash");
args->addoption("recsize", "recsize", 65536/4, "maximum record size", "recsize");
args->addoption("infile", 0, "-", "input file (- - for stdin)", "input");
if (args->interpret(1)!=0) return -1;

infile=args->getstringopt("infile");
recsize=args->getintopt("recsize");
hash=args->getintopt("hash");

return(0);
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
  cout<<"evstream: open "<<direction<<" ("<<fname<<"): "<<strerror(errno)<<endl;
  return -1;
  }
if (ioctl(path, MTIOCTOP, &mt_com)==0)
  {
  iotype=iotype_tape;
  cout<<"evstream: "<<direction<<" is a tape."<<endl;
  }
else
  {
  iotype=iotype_file;
  cout<<"evstream: "<<direction<<" is a pipe or file."<<endl;
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
  cout<<"evstream: "<<direction<<" is a socket: my name: "<<my_address<<endl;
  len=sizeof(struct sockaddr);
  if (getpeername(inf, &peer_address, &len)==0)
    {
    cout<<"  peer name: "<<peer_address<<endl;
    }
  else
    cout<<"getpeername("<<direction<<"): "<<strerror(errno)<<endl;
  }
else
  {
  if (errno==ENOTSOCK)
    { // path is not a socket
    if (ioctl(path, MTIOCTOP, &mt_com)==0)
      { // path is a tape
      iotype=iotype_tape;
      cout<<"evstream: "<<direction<<" is a tape."<<endl;
      }
    else
      { // path is a pipe or regular file
      iotype=iotype_file;
      cout<<"evstream: "<<direction<<" is a pipe or file."<<endl;
      }
    }
  else
    { // it is a socket, but we know nothing about it
    iotype=iotype_socket; // may be bad (?)
    cout<<"getsockname("<<direction<<"): "<<strerror(errno)<<endl;
    }
  }
return 0;
}
/*****************************************************************************/
int do_open()
{
// determine type of stream
if (infile=="-")
  { // stdin
  inf=0;                               // assume that stdin is already open ;-)
  do_stdio_open("input", inf, i_type); // But is it a simple pipe or a socket?
  }
else
  { // regular file, fifo or tape; tape will be detected later
  if (do_file_open(0, "input", infile, inf, i_type)<0) return -1;
  }

switch (i_type)
  {
  case iotype_tape:
    evinput=new C_evtinput(inf, recsize);
    break;
  case iotype_file:
    evinput=new C_evfinput(inf, recsize);
    break;
  case iotype_socket:
    evinput=new C_evpinput(inf, recsize);
    break;
  }
return 0;
}
/*****************************************************************************/
int do_close()
{
delete evinput;
if (inf>0) close(inf);
return 0;
}
/*****************************************************************************/
void do_test()
{
C_eventp inevent;
C_subeventp subevent;
int weiter=1, evcount=0;;
float zsum=0;

while (weiter && ((*evinput) >> inevent))
  {
  struct timeval tv;
  gettimeofday(&tv, 0);
  if (inevent.evnr()==0)
    {
    cout<<"evstream: "<<"userrecord; ignored"<<endl;
    }
  else
    {
    while (inevent >> subevent)
      {
      if (subevent.id()==222)
        {
        float zeit=tv.tv_sec-subevent[2]+(tv.tv_usec-subevent[3])/1000000.;
        zsum+=zeit;
        evcount++;
        if (evcount==1000)
          {
          cout<<zsum/evcount<<" s"<<endl;
          evcount=0;
          zsum=0;
          }
        break;
        }
      }
    }
  }
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  if (do_open()<0) return 1;
  do_test();
  do_close();

  return(0);
  }
catch(C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  return -1;
  }
}
/*****************************************************************************/
/*****************************************************************************/
