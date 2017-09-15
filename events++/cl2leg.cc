/******************************************************************************
*                                                                             *
* cl2leg.cc                                                                   *
*                                                                             *
* created: 22.07.1997                                                         *
* last changed: 06.12.1997                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
#include "config.h"

#ifdef HAVE_STRING_H
#include <String.h>
#define STRING String
#else
#include <string>
#define STRING string
#endif

#define _SOCKADDR_LEN
#include <iostream.h>
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
#include <events.hxx>
#include <swap_int.h>
#include <sockets.hxx>
#include "gnuthrow.hxx"
#include <versions.hxx>

VERSION("Dec 08 1997", __FILE__, __DATE__, __TIME__,
"$ZEL: cl2leg.cc,v 1.4 2005/02/11 15:44:52 wuestner Exp $")

C_readargs* args;
STRING infile, outfile;
int debug, verbose, swap_out;
int recsize, vednum, cl_idx, ev_idx;
int inf, outf;
typedef enum {iotype_file, iotype_tape, iotype_socket} io_types;
io_types i_type, o_type;
C_evoutput* evoutput;
struct event_entry {event_entry* next; C_event* event; int veds;};
event_entry *events=0, *last_event=0;
int last_event_sent=0, last_event_ok=0, last_event_found=0;
sock *insocket, *outsocket;
int* (*read_cluster)();

/*
 * Aufbau eines Clusters: (Element 'data' in struct Cluster)
 * 
 * [0] size (Anzahl aller folgenden Worte)
 * [1] endientest (==0x12345678)
 * [2] type (0: events; 1...: userrecords; 0x10000000: no_more_data)
 * [3] optionsize  (z.B. 4)
 * [4, wenn optionsize!=0] optionflags (1: timestamp 2: checksum 4: ...)
 * [5, wenn vorhanden] timestamp (struct timeval)
 * [5 o. 6, wenn vorhanden] checksum
 * ab hier nur fuer events gueltig
 * [4+optsize] flags ()
 * [5+optsize] ved_id
 * [6+optsize] fragment_id
 * [7+optsize] num_events
 * [8+optsize] size of first event
 * [9+optsize] first event_id
 * ...
 */

typedef enum {clusterty_events=0, clusterty_ved_info, clusterty_text,
    clusterty_no_more_data=0x10000000} clustertypes;
typedef enum {clusterfl_x=0} clusterflags;
#define ClOPTFL_TIME 1
#define ClOPTFL_CHECK 2

/* Makros, um auf Cluster.data zuzugreifen */
#define ClLEN(d)     ((d)[0])
#define ClENDIEN(d)  ((d)[1])
#define ClTYPE(d)    ((d)[2])
#define ClOPTSIZE(d) ((d)[3])
/* Achtung! Cluster darf nicht geswapped sein */
#define ClFLAGS(d)   ((d)[4+ClOPTSIZE(d)])
#define ClVEDID(d)   ((d)[5+ClOPTSIZE(d)])
#define ClFRAGID(d)  ((d)[6+ClOPTSIZE(d)])
#define ClEVNUM(d)   ((d)[7+ClOPTSIZE(d)])
#define ClEVSIZ1(d)  ((d)[8+ClOPTSIZE(d)])
#define ClEVID1(d)   ((d)[9+ClOPTSIZE(d)])
/* fuer geswappte Cluster */
#define ClsFLAGS(d, o)   ((d)[4+(o)])
#define ClsVEDID(d, o)   ((d)[5+(o)])
#define ClsFRAGID(d, o)  ((d)[6+(o)])
#define ClsEVNUM(d, o)   ((d)[7+(o)])
#define ClsEVSIZ1(d, o)  ((d)[8+(o)])
#define ClsEVID1(d, o)   ((d)[9+(o)])

/*****************************************************************************/

void printheader(ostream& os)
{
os<<"Converts EMS cluster format into EMS legacy format."<<endl<<endl;
}

/*****************************************************************************/

void printstreams(ostream& os)
{
os<<endl;
os<<"input and output may be: "<<endl;
os<<"  - for stdin/stdout ;"<<endl;
os<<"  <filename> for regular files or tape;"<<endl;
os<<"  :<port#> or <host>:<port#> for tcp sockets;"<<endl;
os<<"  :<name> for unix sockets;"<<endl;
}

/*****************************************************************************/

int readargs()
{
args->helpinset(3, printheader);
args->helpinset(5, printstreams);

args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");

args->addoption("recsize", "recsize", 65536, "maximum record size (tape only)",
    "recsize");
args->addoption("swap", "swap", false, "swap output", "swap");
args->addoption("vednum", "vednum", 1, "number of VEDs (obsolete)", "vednum");
args->addoption("verbose", "verbose", false, "put out some comments", "verbose");
args->addoption("debug", "debug", false, "debug info", "debug");

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
recsize=args->getintopt("recsize")/4;
vednum=args->getintopt("vednum");
swap_out=args->getboolopt("swap");
debug=args->getboolopt("debug");
verbose=debug||args->getboolopt("verbose");
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
if (debug) cerr<<"do_sock_open("<<direction<<")"<<endl;
int active, portnum;
STRING host, port;

#ifdef HAVE_STRING_H
int colon=fname.index(":");
active=colon!=0;
host=fname(0, colon);
port=fname(colon+1, fname.length());
#else
string::size_type colon=fname.find(':');
active=colon!=0;
host=fname.substr(0, colon);
port=fname.substr(colon+1);
#endif

if (debug) cerr<<"host=>"<<host<<"<; port=>"<<port<<"<"<<endl;
#ifdef HAVE_STRING_H
size_t nums=strspn(port, "0123456789");
#else
string::size_type nums=port.find_first_not_of("0123456789");
#endif
try
  {
  if (nums==port.length())
    {
#ifdef HAVE_STRING_H
    istrstream st((char*)port);
#else
    istrstream st(port.c_str());
#endif
    st>>portnum;
    if (verbose)
      {
      cerr<<"connecting "<<direction<<" with ";
      if (active)
        cerr<<host<<':'<<portnum<<endl;
      else
        cerr<<"port "<<portnum<<endl;
      }
    tcp_socket* socke=new tcp_socket(direction);
    socke->create();
    if (active)
      {
#ifdef HAVE_STRING_H
      socke->connect(host, portnum);
#else
      socke->connect(host.c_str(), portnum);
#endif
      sockptr=socke;
      }
    else
      {
      socke->bind(portnum);
      socke->listen();
      sockptr=socke->accept(0);
      }
    }
  else
    {
    if (verbose)
      {
      cerr<<"connecting "<<direction<<" with socket "<<port;
      if (active)
        cerr<<" (active)"<<endl;
      else
        cerr<<" (passive)"<<endl;
      }
    unix_socket* socke=new unix_socket(direction);
    socke->create();
    if (active)
      {
#ifdef HAVE_STRING_H
      socke->connect(port);
#else
      socke->connect(port.c_str());
#endif
      sockptr=socke;
      }
    else
      {
#ifdef HAVE_STRING_H
      socke->bind(port);
#else
      socke->bind(port.c_str());
#endif
      socke->listen();
      sockptr=socke->accept(0);
      }
    }
  path=sockptr->path();
  }
catch (C_Error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  return -1;
  }
if (verbose)
  {
  struct sockaddr peer_address;
  int len=sizeof(struct sockaddr);
  if (getpeername(path, &peer_address, &len)==0)
    {
    cerr<<"peer name ("<<direction<<"): "<<peer_address<<endl;
    }
  else
    cerr<<"getpeername("<<direction<<"): "<<strerror(errno)<<endl;
  }
return 0;
}
/*****************************************************************************/
int do_file_open(int dir, const char* direction, STRING& fname, int& path,
    io_types& iotype)
{
if (debug) cerr<<"do_file_in_open"<<endl;
struct mtop mt_com;
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;
#ifdef HAVE_STRING_H
if (dir==0)
  path=open((char*)fname, O_RDONLY, 0);
else
  path=open((char*)fname, O_WRONLY|O_CREAT|O_TRUNC, 0640);
#else
if (dir==0)
  path=open(fname.c_str(), O_RDONLY, 0);
else
  path=open(fname.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0640);
#endif
if (path<0)
  {
  cerr<<"open "<<direction<<" ("<<fname<<"): "<<strerror(errno)<<endl;
  return -1;
  }
if (ioctl(path, MTIOCTOP, &mt_com)==0)
  {
  iotype=iotype_tape;
  if (verbose) cerr<<direction<<" is a tape."<<endl;
  }
else
  {
  iotype=iotype_file;
  if (verbose) cerr<<direction<<" is a pipe or file."<<endl;
  }
}
/*****************************************************************************/
int do_stdio_open(const char* direction, int& path, io_types& iotype)
{
if (debug) cerr<<"do_stdio_open("<<direction<<")"<<endl;
struct mtop mt_com;
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;
struct sockaddr my_address, peer_address;
int len=sizeof(struct sockaddr);
if (getsockname(path, &my_address, &len)==0)
  { // path is a socket
  iotype=iotype_socket;
  if (verbose)
    {
    cerr<<direction<<" is a socket: my name: "<<my_address<<endl;
    len=sizeof(struct sockaddr);
    if (getpeername(inf, &peer_address, &len)==0)
      {
      cerr<<"  peer name: "<<peer_address<<endl;
      }
    else
      cerr<<"getpeername("<<direction<<"): "<<strerror(errno)<<endl;
    }
  }
else
  {
  if (errno==ENOTSOCK)
    { // path is not a socket
    if (ioctl(path, MTIOCTOP, &mt_com)==0)
      { // path is a tape
      iotype=iotype_tape;
      if (verbose) cerr<<direction<<" is a tape."<<endl;
      }
    else
      { // path is a pipe or regular file
      iotype=iotype_file;
      if (verbose) cerr<<direction<<" is a pipe or file."<<endl;
      }
    }
  else
    { // it is a socket, but we know nothing about it
    iotype=iotype_socket; // may be bad (?)
    if (verbose) cerr<<"getsockname("<<direction<<"): "<<strerror(errno)<<endl;
    }
  }
return 0;
}
/*****************************************************************************/
int do_open()
{
insocket=0; outsocket=0;

// determine type of stream
if (infile=="-")
  { // stdin
  inf=0;                               // assume that stdin is already open ;-)
  do_stdio_open("input", inf, i_type); // But is it a simple pipe or a socket?
  }
#ifdef HAVE_STRING_H
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

if (outfile=="-")
  { // stdin
  outf=1;                             // assume that stdout is already open ;-)
  do_stdio_open("ouput", outf, o_type); // But is it a simple pipe or a socket?
  }
#ifdef HAVE_STRING_H
else if (outfile.index(":")>=0)
#else
else if (outfile.find(':')!=string::npos)
#endif
  { // socket
  o_type=iotype_socket;
  if (do_sock_open("output", outfile, outf, outsocket)<0) return -1;
  }
else
  { // regular file, fifo or tape; tape will be detected later
  if (do_file_open(1, "output", outfile, outf, o_type)<0) return -1;
  }
return 0;
}
/*****************************************************************************/

int do_close()
{
if (inf>0) close(inf);
if (outf>1) close(outf);
return 0;
}

/*****************************************************************************/

int write_ev(int* buf)
{
int ev_id=buf[1];
int sevnum=buf[3];
C_event* event;
event_entry* evente=events;

if (ev_id>last_event_found) last_event_found=ev_id;

while ((evente!=0) && (evente->event->evnr()!=ev_id)) evente=evente->next;
if (evente==0)
  {
  event=new C_event;
  if (event==0)
    {
    cerr<<"new C_event("<<recsize<<"): "<<strerror(errno)<<endl;
    return -1;
    }
  evente=new event_entry;
  evente->next=0;
  evente->event=event;
  if (events==0)
    events=evente;
  else
    last_event->next=evente;
  last_event=evente;
  event->evnr(ev_id);
  event->trignr(buf[2]);
  evente->veds=0;
  }
else
  event=evente->event;

buf+=4;
for (int sev=0; sev<sevnum; sev++)
  {
  C_subevent subevent;
  subevent.store(buf);
  (*event)<<subevent;
  buf+=buf[1]+2;
  }
evente->veds++;
if (evente->veds==vednum)
  {
  if (verbose && (ev_id%1000==0)) cerr<<"event "<<ev_id<<" complete"<<endl;
  if (ev_id<=last_event_sent)
    {
    cerr<<"cl2leg: last event="<<last_event_sent<<"; current event="<<ev_id<<endl;
    }
  if (ev_id>last_event_sent) last_event_sent=ev_id;
  if (evente!=events)
    {
    cerr<<"cl2leg: logikfehler in write_ev"<<endl;
    return -1;
    }
  (*evoutput)<<(*event);
  events=evente->next;
  delete event;
  delete evente;
  }
return 0;
}

/*****************************************************************************/

int write_events(int* buf, unsigned int evnum)
{
//cerr<<"will write "<<evnum<<" events"<<endl;
for (int ev=0; ev<evnum; ev++)
  {
  try
    {
    //if (ev%100==0) cerr<<"ev "<<ev<<"; cl="<<cl_idx<<"; sevn="<<buf[3]<<endl;
    //if ((cl_idx==6) && (ev>600))
    //  {
    //  cerr<<ev<<":"<<hex;
    //  for (int i=0; i<10; i++) cerr<<" "<<buf[i];
    //  cerr<<endl<<dec;
    //  }
    if (write_ev(buf)!=0) return -1;
    ev_idx++;
    }
  catch (C_Error* e)
    {
    cerr<<(*e)<<endl;
    delete e;
    return -1;
    }
  buf+=buf[0]+1;
  }
return 0;
}

/*****************************************************************************/
int got_events(int* buf)
{
if (verbose) cerr<<"cl2leg: got events"<<endl;
if (debug)
  {
  int num=buf[0]+1; if (num>20) num=20;
  cerr<<hex;
  for (int i=0; i<num; i++) cerr<<buf[i]<<" ";
  cerr<<dec<<endl;
  }
int optsize=buf[3];
int* data=buf+4+optsize;
int flags=*data++;
int ved_id=*data++;
int fragment_id=*data++;
int num_events=*data++;
if (verbose)
  {
  cerr<<"Cluster "<<cl_idx<<"; ved "<<ved_id<<"; events "<<data[1]<<"..."
      <<data[1]+num_events-1<<endl;
  }
if (flags!=0) {cerr<<"cl2leg: Cluster "<<cl_idx<<": flags=0x"<<hex<<flags<<dec<<endl;}
if (fragment_id!=0)
  {
  cerr<<"cl2leg: Cluster "<<cl_idx<<": fragment_id="<<fragment_id<<endl;
  }
if (flags!=0) return -1;
return write_events(data, num_events);
}
/*****************************************************************************/
void got_text(int* buf)
{
if (verbose) cerr<<"cl2leg: got text; not decoded."<<endl;
}
/*****************************************************************************/
void got_ved_info(int* buf)
{
if (verbose) cerr<<"cl2leg: got VED info"<<endl;
if (debug)
  {
  cerr<<hex;
  for (int i=0; i<buf[0]+1; i++) cerr<<buf[i]<<" ";
  cerr<<dec<<endl;
  }
int optsize=buf[3];
int* data=buf+4+optsize;
vednum=data[0];
int numis=data[1];
int* isids=new int[numis];
for (int i=0; i<numis; i++) isids[i]=data[2+i];
if (verbose)
  {
  cerr<<"  "<<vednum<<" VEDs; "<<numis<<" ISs: ";
  for (int i=0; i<numis; i++) cerr<<isids[i]<<" ";
  cerr<<endl;
  }
}
/*****************************************************************************/
int* read_cluster_tape()
{
int da, size, wenden;
int *buf=new int[recsize];
if (buf==0)
  {
  cerr<<"new int["<<recsize<<"]: "<<strerror(errno)<<endl;
  return 0;
  }
da=read(inf, (char*)buf, recsize*4);
if (da<0)
  {
  cerr<<"read tape: "<<strerror(errno)<<endl;
  delete[] buf;
  return 0;
  }
else if (da==0)
  {
  cerr<<"read filemark"<<endl;
  delete[] buf;
  return 0;
  }
else
  {
  if (da&3)
    {
    cerr<<"Cluster "<<cl_idx<<": recsize="<<da<<endl;
    delete[] buf;
    return 0;
    }
  if (buf[1]==0x12345678) // host ordering
    wenden=0;
  else if (buf[1]==0x78563412) // verkehrt
    wenden=1;
  else // unbekannt
    {
    cerr << "read new cluster: byteorder="<<hex<<buf[1]<<dec<<endl;
    delete[] buf;
    return 0;
    }
  if (debug)
    {
    cerr<<hex;
    for (int i=0; i<12; i++) cerr<<buf[i]<<" ";
    cerr<<dec<<endl;
    }
  size=da>>2;
  if (wenden) for (int i=0; i<size; i++) buf[i]=swap_int(buf[i]);
  if (size!=buf[0]+1)
    {
    cerr<<"Cluster "<<cl_idx<<": recordsize="<<size<<"; clustersize="<<buf[0]+1
        <<endl;
    cerr<<hex;
    for (int i=0; i<12; i++) cerr<<buf[i]<<" ";
    cerr<<dec<<endl;
    }
  }
return buf;
}
/*****************************************************************************/
int xrecv(int path, int* buf, int len)
{
int da, restlen;
char *bufptr;

restlen=len*sizeof(int);
bufptr=(char*)buf;
while (restlen)
  {
  da=read(path, bufptr, restlen);
  if (da>0)
    {
    bufptr+=da;
    restlen-=da;
    }
  else if (da==0)
    {
    cerr<<"xrecv: broken pipe?"<<endl;
    return -1;
    }
  else
    {
    if (errno!=EINTR)
      {
      cerr<<"xrecv: "<<strerror(errno)<<endl;
      return -1;
      }
    }
  }
return 0;
}
/*****************************************************************************/
int* read_cluster_file()
{
int *buf, kopf[2], size, wenden;
// header lesen
if (xrecv(inf, kopf, 2)!=0) return 0;
// endian testen
if (debug) cerr<<hex<<"k0="<<kopf[0]<<"; k1="<<kopf[1]<<dec<<endl;
if (kopf[1]==0x12345678) // host ordering
  {
  wenden=0;
  size=kopf[0]+1;
  }
else if (kopf[1]== 0x78563412) // verkehrt
  {
  wenden=1;
  size=swap_int(kopf[0])+1;
  }
else // unbekannt
  {
  cerr << "read new cluster: byteorder="<<hex<<kopf[1]<<dec<<endl;
  return 0;
  }
buf=new int[size];
if (buf==0)
  {
  cerr<<"new int["<<size<<"]: "<<strerror(errno)<<endl;
  return 0;
  }
buf[0]=kopf[0];
buf[1]=kopf[1];
if (xrecv(inf, buf+2, size-2)!=0)
  {
  delete[] buf;
  return 0;
  }
if (debug)
  {
  cerr<<hex;
  for (int i=0; i<12; i++) cerr<<buf[i]<<" ";
  cerr<<dec<<endl;
  }
if (wenden)
  for (int i=0; i<size; i++) buf[i]=swap_int(buf[i]);
return buf;
}
/*****************************************************************************/
int do_convert()
{
int *buf, type;
cl_idx=0; ev_idx=0;

do
  {
  buf=read_cluster();
  if (buf==0) break;
  switch (type=ClTYPE(buf)/*cluster type*/)
    {
    case clusterty_events:
      if (got_events(buf)<0) goto fehler;
      break;
    case clusterty_ved_info:
      got_ved_info(buf);
      break;
    case clusterty_text:
      got_text(buf);
      break;
    case clusterty_no_more_data:
      if (verbose) cerr<<"cl2leg: End of Data received"<<endl;
      break;
    default:
      cerr<<"cl2leg: unknown cluster type "<<type<<endl;
      goto fehler;
    }
  cl_idx++;
  delete[] buf;
  }
while (type!=clusterty_no_more_data);
if (verbose) cerr<<"cl2leg: "<<cl_idx<<" cluster converted ("<<cl_idx
    <<" events)"<<endl;
return 0;
fehler:
return -1;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);
  if (do_open()<0) {do_close(); return 1;}
  switch (o_type)
    {
    case iotype_tape:
      if (debug) cerr<<"cl2leg: output to tape"<<endl;
      evoutput=new C_evtoutput(outf, recsize);
      break;
    case iotype_file:
      if (debug) cerr<<"cl2leg: output to file"<<endl;
      evoutput=new C_evfoutput(outf, recsize);
      break;
    case iotype_socket:
      if (debug) cerr<<"cl2leg: output to socket"<<endl;
      evoutput=new C_evpoutput(outf, recsize);
      break;
    }
  evoutput->swap(swap_out);
  switch (i_type)
    {
    case iotype_tape:
      if (debug) cerr<<"cl2leg: input from tape"<<endl;
      read_cluster=read_cluster_tape;
      break;
    case iotype_file:
      if (debug) cerr<<"cl2leg: input from file"<<endl;
      read_cluster=read_cluster_file;
      break;
    case iotype_socket:
      if (debug) cerr<<"cl2leg: input from socket"<<endl;
      read_cluster=read_cluster_file;
      break;
    }
  do_convert();
  delete evoutput;
  do_close();
  return 0;
  }
catch (C_Error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  return -1;
  }
}

/*****************************************************************************/
/*****************************************************************************/
