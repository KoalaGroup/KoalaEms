/*
 * event_deflector.cc
 * 
 * created: 29.Jan.2001 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sockets.hxx>
#include <errors.hxx>
#include <versions.hxx>

VERSION("Feb 01 2001", __FILE__, __DATE__, __TIME__,
"$ZEL: event_deflector.cc,v 1.6 2004/11/26 14:40:16 wuestner Exp $")
#define XVERSION

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

C_readargs* args;

class databuf {
  public:
    databuf(int size);
    ~databuf();
    void incr() {linkcount++;}
    void decr();
  protected:
    int linkcount;
  public:
    int size;
    unsigned int* buf;
};
databuf* newdatabuf;

class sockdescr {
  public:
    sockdescr(sock* path, databuf* fbuf);
    ~sockdescr();
    int do_read();
    int do_write();
  protected:
    unsigned int idx;
  public:
    databuf* fbuf;
    sock* path;
};

sockdescr* insock;
sockdescr** outsock;
int numoutsocks;

typedef enum {clusterty_events=0, clusterty_ved_info, clusterty_text,
    clusterty_wendy_setup,
    clusterty_no_more_data=0x10000000} clustertypes;

int bigendian;
int copy_empty;

/*****************************************************************************/
int readargs()
{
args->addoption("inport", "iport", 11110, "Port for incoming clusters",
    "inport");
args->addoption("outport", "oport", 11111, "Port for outgoing events",
    "outport");

args->addoption("isid", "isid", 0, "instrumentation system ID", "isid");
args->addoption("empty", "empty", false, "do not supress empty events", "empty");

args->multiplicity("isid", 0);

if (args->interpret(1)) return -1;
copy_empty=args->getboolopt("empty");
return 0;
}
/*****************************************************************************/
extern "C" {
void inthand(int sig)
{}
}
/*****************************************************************************/
databuf::databuf(int size)
:linkcount(0), size(size)
{
    buf=new unsigned int[size];
}
/*****************************************************************************/
databuf::~databuf(void)
{
delete[] buf;
}
/*****************************************************************************/
void databuf::decr()
{
linkcount--;
if (!linkcount) delete this;
}
/*****************************************************************************/
sockdescr::sockdescr(sock* path, databuf* fbuf)
:idx(0), fbuf(fbuf), path(path)
{
    if (fbuf)
        fbuf->incr();
}
/*****************************************************************************/
sockdescr::~sockdescr(void)
{
if (fbuf) fbuf->decr();
delete path;
}
/*****************************************************************************/
int xread(int s, char* buf, int count)
{
int idx=0, res;
while (count)
  {
  res=read(s, buf+idx, count);
  if (res<0)
    {
    if (errno!=EINTR)
      {
      cerr<<"xread(count="<<count<<"): "<<strerror(errno)<<endl;
      return -1;
      }
    }
  else if (res==0) /* broken pipe */
    {
    /*cerr<<"xread(count="<<count<<"): read 0 bytes"<<endl;*/
    return -1;
    }
  else
    {
    idx+=res;
    count-=res;
    }
  }
return 0;
}
/*****************************************************************************/
int sockdescr::do_read()
{
unsigned int head[2], *cluster;
int wenden, swap, size, evsize, evidx, evnum, n, p;

if (xread(*path, (char*)head, 2*sizeof(int))<0) return -1;
switch (head[1])
  {
  case 0x12345678: wenden=0; break;
  case 0x78563412: wenden=1; break;
  default:
    cerr<<"endian="<<hex<<head[1]<<dec<<endl;
    return -1;
  }
size=wenden?swap_int(head[0]):head[0];
cluster=new unsigned int[size-1];
if (!cluster)
  {
  cerr<<"new cluster("<<size-1<<"): "<<strerror(errno)<<endl;
  return -1;
  }
if (xread(*path, (char*)cluster, (size-1)*sizeof(int))<0)
  {
  delete[] cluster;
  return -1;
  }
if (cluster[0]!=0) /* clusterty_events?*/
  {
  delete[] cluster;
  return 0;
  }
n=wenden?swap_int(cluster[1]):cluster[1]; /* size of options */
p=n+5; /* skip options, flags, VED_ID and fragment_id */
evnum=wenden?swap_int(cluster[p]):cluster[p]; /* number of events */
if (evnum==0)
  {
  cerr<<"cluster contains no events"<<endl;
  delete[] cluster;
  return 0;
  }
p++; /* start of first event */
if (evnum!=1)
/* jump to the last event */
  {
  for (int e=1; e<evnum; e++)
    {
    evsize=wenden?swap_int(cluster[p]):cluster[p];
    p+=evsize+1;
    }
  }
evsize=wenden?swap_int(cluster[p]):cluster[p]; /* size of event */
evidx=wenden?swap_int(cluster[p+1]):cluster[p+1]; /* event index */
/*cerr<<"EVENT "<<evidx<<endl;*/

if ((evsize>3) || copy_empty)
  {
  fbuf=new databuf(evsize+1);
  if (!fbuf)
    {
    cerr<<"new databuf("<<evsize+1<<"): "<<strerror(errno)<<endl;
    delete[] cluster;
    return 0;
    }
  swap=bigendian==wenden;
  if (swap)
    {
    for (int i=0; i<=evsize; i++) fbuf->buf[i]=swap_int(cluster[p+i]);
    }
  else
    {
    for (int i=0; i<=evsize; i++) fbuf->buf[i]=cluster[p+i];
    }
  fbuf->incr();
  if (newdatabuf) newdatabuf->decr();
  newdatabuf=fbuf;
  fbuf=0;
  for (int i=0; i<numoutsocks; i++)
    {
    if (!outsock[i]->fbuf)
      {
      outsock[i]->fbuf=newdatabuf;
      newdatabuf->incr();
      }
    }
  }
delete[] cluster;
return 0;
}
/*****************************************************************************/
int sockdescr::do_write()
{
    ssize_t res;
    res=write(*path, (char*)(fbuf->buf)+idx, fbuf->size*sizeof(int)-idx);
    if (res<=0) {
        if ((res<0) && (errno==EINTR)) return 0;
        if (res==0) errno=EPIPE;
        cerr<<"do_write: "<<strerror(errno)<<endl;
        return -1;
    }
    idx+=res;
    if (idx==fbuf->size*sizeof(int)) {
        // databuf* newbuf=newdatabuf;
        fbuf->decr();
        idx=0;
        if (newdatabuf!=fbuf) {
            fbuf=newdatabuf;
            fbuf->incr();
        } else {
            fbuf=0;
        }
    }
    return 0;
}
/*****************************************************************************/
void do_accept_server_outsock(tcp_socket& server_outsock)
{
sock* newsock;
try
  {
  newsock=server_outsock.sock_accept(1);
  }
catch (C_error* e)
  {
  cerr<<"accept_server_outsock: "<<*e<<endl;
  delete e;
  return;
  }
sockdescr** help;
try
  {
  help=new sockdescr*[numoutsocks+1];
  if (!help) throw new C_unix_error(errno, "");
  sockdescr* newdescr=new sockdescr(newsock, newdatabuf);
  if (!help) throw new C_unix_error(errno, "");

  for (int i=0; i<numoutsocks; i++) help[i]=outsock[i];
  help[numoutsocks++]=newdescr;
  delete[] outsock;
  outsock=help;
  }
catch (C_error* e)
  {
  cerr<<"accept_server_outsock: "<<*e<<endl;
  delete e;
  }
catch (...)
  {
  cerr<<"caught '...' in accept_server_outsock"<<endl;
  }
/*cerr<<"new outsock ("<<(int)(*newsock)<<") accepted"<<endl;*/
}
/*****************************************************************************/
void delete_outsock(int idx)
{
    //int path=*(outsock[idx]->path);
    delete outsock[idx];
    for (int j=idx; j<numoutsocks-1; j++)
        outsock[j]=outsock[j+1];
    numoutsocks--;
    /*cerr<<"outsock("<<path<<") deleted"<<endl;*/
}
/*****************************************************************************/
void do_accept_server_insock(tcp_socket& server_insock)
{
sock* newinsock;
try
  {
  newinsock=server_insock.sock_accept(0);
  }
catch (C_error* e)
  {
  cerr<<"accept_server_insock: "<<*e<<endl;
  delete e;
  return;
  }
if (insock)
  {
  cerr<<"accept_server_insock: already connected"<<endl;
  delete newinsock;
  return;
  }
try
  {
  insock=new sockdescr(newinsock, 0);
  if (!insock) throw new C_unix_error(errno, "");
  }
catch (C_error* e)
  {
  cerr<<"accept_server_outsock: "<<*e<<endl;
  delete e;
  }
catch (...)
  {
  cerr<<"caught '...' in accept_server_outsock"<<endl;
  insock=0;
  }
/*cerr<<"new insock accepted"<<endl;*/
}
/*****************************************************************************/
void dump_fds(char* txt, int nfds, fd_set* readfds, fd_set* writefds)
{
int i;
cerr<<txt<<": nfds="<<nfds<<endl;
cerr<<"  readfds:";
for (i=0; i<nfds; i++)
  {
  if (FD_ISSET(i, readfds)) cerr<<" "<<i;
  }
cerr<<endl;
cerr<<"  writefds:";
for (i=0; i<nfds; i++)
  {
  if (FD_ISSET(i, writefds)) cerr<<" "<<i;
  }
cerr<<endl;
}
/*****************************************************************************/
void main_loop(tcp_socket& server_insock, tcp_socket& server_outsock)
{
/*
cerr<<"server_insock_path="<<(int)server_insock<<endl;
cerr<<"server_outsock_path="<<(int)server_outsock<<endl;
*/
while (1)
  {
  fd_set readfds, writefds;
  int res, nfds, i;

  /*cerr<<"-------------------------------------------------------"<<endl;*/
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_SET(server_insock, &readfds);
  nfds=server_insock;
  FD_SET(server_outsock, &readfds);
  if (nfds<server_outsock) nfds=server_outsock;

  if (insock)
    {
    FD_SET(*insock->path, &readfds);
    if (nfds<*insock->path) nfds=*insock->path;
    }
  for (i=0; i<numoutsocks; i++)
    {
    if (outsock[i]->fbuf) FD_SET(*outsock[i]->path, &writefds);
    if (nfds<*outsock[i]->path) nfds=*outsock[i]->path;
    }

  nfds++;
  /*dump_fds("before select", nfds, &readfds, &writefds);*/
  res=select(nfds, &readfds, &writefds, (fd_set*)0, (struct timeval*)0);
  /*dump_fds("after select", nfds, &readfds, &writefds);*/
  if (res<=0)
    {
    cerr<<"select: res="<<res<<"; : "<<strerror(errno)<<endl;
    }
  else
    {
    if (insock)
      {
      if (FD_ISSET(*insock->path, &readfds))
        {
        if (insock->do_read()<0)
          {
          /*cerr<<"INSOCK CLOSED!"<<endl;*/
          delete insock;
          insock=0;
          }
        }
      }
    if (FD_ISSET(server_insock, &readfds))
      {
      do_accept_server_insock(server_insock);
      }
    if (FD_ISSET(server_outsock, &readfds))
      {
      do_accept_server_outsock(server_outsock);
      }
    i=0;
    while (i<numoutsocks)
      {
      int res=0;
      if (FD_ISSET(*outsock[i]->path, &writefds))
        {
        /* do_write returns -1 if outsock[i] has to be deleted */
        if ((res=outsock[i]->do_write())<0) delete_outsock(i);
        }
      if (!res) i++;
      }
    }
  }
}
/*****************************************************************************/
int checkendian()
{
int endian;

union
  {
  unsigned int i;
  unsigned char b[4];
  } t;
t.b[0]=0x12;
t.b[1]=0x34;
t.b[2]=0x56;
t.b[3]=0x78;
switch (t.i)
  {
  case 0x78563412: endian=0; break;
  case 0x12345678: endian=1; break;
  default:
    cerr<<"unknown endian of myself: "<<hex<<t.i<<dec<<endl;
    endian=-1;
  }
return endian;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
tcp_socket sourcesocket("source");
tcp_socket destsocket("dest");

args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

if ((bigendian=checkendian())<0) return 1;

struct sigaction vec, ovec;

if (sigemptyset(&vec.sa_mask)==-1)
  {
  cerr << "sigemptyset: " << strerror(errno) << endl;
  return(-1);
  }
vec.sa_flags=0;
vec.sa_handler=inthand;
if (sigaction(SIGPIPE, &vec, &ovec)==-1)
  {
  cerr << "sigaction: " << strerror(errno) << endl;
  return(-1);
  }
  
try
  {
  sourcesocket.create();
  sourcesocket.bind(args->getintopt("inport"));
  sourcesocket.listen();
  destsocket.create();
  destsocket.bind(args->getintopt("outport"));
  destsocket.listen();
  }
catch(C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  return 0;
  }

newdatabuf=0;
outsock=0;
numoutsocks=0;

main_loop(sourcesocket, destsocket);
}

/*****************************************************************************/
/*****************************************************************************/
