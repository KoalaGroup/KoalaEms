/*
 * parasit.cc
 * created: 20.01.95 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <xdrstrdup.hxx>
#include <ved_addr.hxx>
#include <inbuf.hxx>
#include <outbuf.hxx>
#include <client_defaults.hxx>
#include <versions.hxx>

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: parasit.cc,v 2.15 2007/04/18 20:05:14 wuestner Exp $")
#define XVERSION

C_VED* em;
int do_idx;
C_readargs* args;
int verbose;
int debug;
pid_t reader;
short port;
const char* vedname;
const char* hostname;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK,
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("nosearch", "nosearch", false,
    "don't search for commu and VED");
args->addoption("vedname", 0, "", "Name des VED", "vedname");
args->addoption("reader", "reader", "evsave", "Pfad zum Reader", "reader");
args->addoption("readerargs", "args", "", "Arguments for reader", "readerargs");
args->addoption("readerport", "rport", 4321, "Port for reader", "readerport");
args->addoption("help", "help", false, "this text");
args->addoption("buffersize", "buffersize", 300000, "Buffersize", "buffersize");
args->addoption("readerhost", "rhost", "", "Host for reader",
    "readerhost");
args->addoption("prior", "prior", -1, "Priority", "priority");
args->hints(C_readargs::required, "vedname", 0);
args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
args->hints(C_readargs::exclusive, "port", "sockname", 0);
return args->interpret(1);
}

/*****************************************************************************/

int search_ved()
{
int port, lastport, foundport;
C_VED_addr* addr;

addr=0;
if (verbose) cout << "search for VED " << vedname << endl;
lastport=port=args->getintopt("port");
while ((port-lastport)<10)
  {
  try // 01
    {
    C_VED* commu;
    C_confirmation* conf;
    C_instr_system* commu_is;

    if (verbose) cout << "try port " << port << endl;
    communication.init(hostname, port);
    lastport=port;
    commu=new C_VED("commu");
    commu_is=commu->is0();
    try // 02
      {
      if (debug) cout << "ask wether VED is open" << endl;
      conf=commu_is->command("VEDinfo", vedname);
      // VED offen vorgefunden, wird benutzt;
      C_inbuf inbuf(conf->buffer(), conf->size());
      delete conf;
      inbuf+=6;
      addr=extract_ved_addr(inbuf);
      cout << "ved found: commu-port: " << port << ";" << endl << "ved-addr: "
          << addr << endl;
      delete commu;
      em=new C_VED(vedname);
      return(0);
      } // t02
    catch (C_ved_error* e) // 02a
      {
      // VED nicht offen
      delete e;
      int ok=1;
      try // 03
        {
        if (debug) cout << "ask wether VED is known" << endl;
        conf=commu_is->command("VEDaddress", vedname);
        } // t03
      catch (C_error* e) // 03
        {
        cerr << "port " << port << ": " << (*e) << endl;
        delete e;
        ok=0;
        } // c03
      if (ok)
        {
        C_inbuf inbuf(conf->buffer(), conf->size());
        delete conf;
        inbuf++;
        if (inbuf.getint())
          {
          C_VED_addr* addr_;
          int ok=1;
          try // 04
            {
            addr_=extract_ved_addr(inbuf);
            } // t04
          catch (C_error* e) // 04
            {
            cerr << "port " << port << ": " << (*e) << endl;
            delete e;
            ok=0;
            } // c04
          if (ok)
            {
            if (addr && (*addr!=*addr_))
              {
              cerr << "found different addresses for ved " << vedname << ":"
                  << endl
                  << " port " << foundport << ": " << addr << endl
                  << " port " << port << ": " << addr_ << endl;
              delete commu; delete conf; delete addr;
              return(-1);
              }
            if (verbose) {cout << "commu knows ved: " << addr_ << endl;}
            if (addr)
              delete addr_;
            else
              {
              foundport=port;
              addr=addr_;
              }
            }
          }
        }
      delete commu;
      } // c02a
    catch (C_error* e) // 02b
      {
      cerr << "port " << port << ": " << (*e) << endl;
      delete e;
      delete commu;
      } // 02b
    }
  catch (C_error* e) // 01
    {
    if (debug) cout << (*e) << endl;
    delete e;
    }
  if (communication.valid()) communication.done();
  port++;
  }
if (addr==0)
  {
  cerr << "VED " << vedname << " not found" << endl;
  return(-1);
  }

// an dieser Stelle wissen wir, dass VED bekannt ist, aber nicht geoffnet
// wir haben eine eindeutige Adresse, VED kann aber unter irgendeinem
// Namen geoeffnet sein

if (verbose) cout << "search for open ved " << addr << endl;
lastport=port=args->getintopt("port");
while ((port-lastport)<10)
  {
  try // 01
    {
    C_VED* commu;
    C_instr_system* commu_is;
    C_confirmation* conf;
    int num;

    if (verbose) cout << "try port " << port << endl;
    communication.init(hostname, port);
    lastport=port;
    commu=new C_VED("commu");
    commu_is=commu->is0();

    if (debug) cout << "ask wether commu knows VED_addr" << endl;
    C_outbuf outbuf;
    outbuf << *addr;
    conf=commu_is->command("VEDnamebyaddress", outbuf);
    C_inbuf inbuf(conf->buffer(), conf->size());
    delete conf;
    inbuf++; // errorcode
    inbuf >> num;
    if (debug) cout << "num=" << num << endl;

    int i;
    for (i=0; i<num; i++)
      {
      STRING s;
      inbuf >> s;

      try
        {
        if (debug) cout << "ask wether VED " << s << " is open" << endl;
#ifdef USE_STRING_H
        delete commu_is->command("VEDinfo", s);
#else
        delete commu_is->command("VEDinfo", s.c_str());
#endif
        // VED offen vorgefunden, wird benutzt;
        cout << "ved found: commu-port: " << port << " name: " << s
            << ";" << endl << "ved-addr: " << addr << endl;
        em=new C_VED(s);
        delete commu;
        return(0);
        }
      catch (C_error* e)
        {
        if (debug) cerr << "port: " << port << ": " << (*e) << endl;
        delete e;
        }
      }
    delete commu;
    }
  catch (C_error* e)
    {
    if (debug) cerr << "port: " << port << ": " << (*e) << endl;
    delete e;
    }
  if (communication.valid()) communication.done();
  port++;
  }
// wir haben immer noch keinen commu gefunden, der fragliches VED
// benutzt, vor lauter Wut nehmen wir den ersten, der VED kennt.

if (verbose) cout << "search for ved " << addr << endl;
lastport=port=args->getintopt("port");
while ((port-lastport)<10)
  {
  try // 01
    {
    C_VED* commu;
    C_instr_system* commu_is;
    C_confirmation* conf;
    int num;

    if (verbose) cout << "try port " << port << endl;
    communication.init(hostname, port);
    lastport=port;
    commu=new C_VED("commu");
    commu_is=commu->is0();

    if (debug) cout << "ask wether commu knows VED_addr" << endl;
    C_outbuf outbuf;
    outbuf << *addr;
    conf=commu_is->command("VEDnamebyaddress", outbuf);
    C_inbuf inbuf(conf->buffer(), conf->size());
    delete conf;
    delete commu;

    inbuf++; // errorcode
    inbuf >> num;
    if (debug) cout << "num=" << num << endl;

    STRING s;
    inbuf >> s;
    if (verbose) cout << "open VED " << s << " via commu on port " << port
        << endl;

    try
      {
      em=new C_VED(s);
      return(0);
      }
    catch (C_error* e)
      {
      if (debug) cerr << "port: " << port << ": " << (*e) << endl;
      delete e;
      }
    }
  catch (C_error* e)
    {
    if (debug) cerr << "port: " << port << ": " << (*e) << endl;
    delete e;
    }
  if (communication.valid()) communication.done();
  port++;
  }

delete addr;
return(-1);
}

/*****************************************************************************/

int open_ved()
{
if (args->getboolopt("nosearch"))
  {
  if (verbose) cout << "no search for commu and VED" << endl;
  try
    {
    if (!args->isdefault("hostname") || !args->isdefault("port"))
      {
      if (verbose) cout << "use host \"" << args->getstringopt("hostname")
          << "\", port " << args->getintopt("port") << endl;
      communication.init(args->getstringopt("hostname"),
          args->getintopt("port"));
      }
    else
      {
      if (verbose) cout << "use socket \""
          << args->getstringopt("sockname") << "\"" << endl;
      communication.init(args->getstringopt("sockname"));
      }
    em=new C_VED(vedname);
    }
  catch (C_error* e)
    {
    cerr << (*e) << endl;
    delete e;
    return(-1);
    }
  return(0);
  }
else // search remote commu and ved
  {
  if (verbose) cout << "search ved" << endl;
  return(search_ved());
  }
}

/*****************************************************************************/

int purge_dataout()
{
if (verbose) cout << "purge dataout" << endl;

try
  {
  C_confirmation* conf;
  int i;
  conf=em->GetNamelist(Object_do);
  int n=conf->buffer(1);
  int* buff=conf->buffer()+2;
  for (i=0; i<n; i++)
    {
    try
      {
      int err;
      C_confirmation* conf;
      if (verbose) cout << "dataout " << i << ":" << endl;
      conf=em->GetDataoutStatus(buff[i]);
      err=conf->buffer(1);
      delete conf;
      if (err!=0)
        {
        if (verbose) cout << "  error " << err << " ("
            << ((err>>8)&0xff) << ":" << (err&0xff) << ")"<< endl;
        C_data_io* addr;
        addr=em->UploadDataout(i);
        if (addr->addrtype()==Addr_Socket)
          {
          delete addr;
          if (verbose) cout << "  try to delete it" << endl;
          em->DeleteDataout(i);
          }
        else
          delete addr;
        }
      }
    catch (C_error* e)
      {
      if (verbose) cerr << (*e) << endl;
      delete e;
      }
    }
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(-1);
  }
return(0);
}

/*****************************************************************************/

int create_dataout()
{
if (purge_dataout()==-1) return(-1);

if (verbose) cout << "create dataout" << endl;

try
  {
  C_confirmation* conf;
  int i, found;
  conf=em->GetNamelist(Object_domain, Dom_Dataout);
  int n=conf->buffer(1);
  int* buff=conf->buffer()+2;
  i=0; do_idx=-1; found=0;
  do
    {
    do_idx++;
    found=1;
    for (i=0; i<n; i++) if (buff[i]==do_idx) found=0;
    }
  while (!found);
  delete conf;
  if (verbose) cout << "try index " << do_idx << endl;
  em->DownloadDataout(do_idx,
      InOut_Cluster,
      args->getintopt("buffersize"),
      args->getintopt("prior"),
      Addr_Socket,
      args->getstringopt("readerhost"),
      args->getintopt("readerport"));
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(-1);
  }
cout << "Dataout (idx=" << do_idx << ") created." << endl;

return(0);
}

/*****************************************************************************/

void test_readout()
{
if (verbose) cout << "test readout" << endl;
try
  {
  InvocStatus stat;
  ems_u32 count;
  stat=em->GetReadoutStatus(&count);
  switch (stat)
    {
    case Invoc_error    : cout << "readout in errorstate" << endl; break;
    case Invoc_alldone  : cout << "no data for readout" << endl; break;
    case Invoc_stopped  : cout << "readout stopped" << endl; break;
    case Invoc_notactive: cout << "readout inactive" << endl; break;
    case Invoc_active   : cout << "readout active; event " << count << endl; break;
    default: cout << "readout in unknown state " << static_cast<int>(stat) << endl; break;
    }
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
}

/*****************************************************************************/

int start_reader()
{
cout << "open reader" << endl;
return(0);
}

/*****************************************************************************/

int stop_reader()
{
//kill(reader, SIGQUIT);
return(0);
}

/*****************************************************************************/

int close_commu()
{
communication.done();
return(0);
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  debug=args->getboolopt("debug");
  verbose=debug || args->getboolopt("verbose");

  vedname=args->getstringopt("vedname");
  hostname=args->getstringopt("hostname");
  if (verbose) cout << "use host " << hostname << endl;

  if (open_ved()==0)
    {
    cout << "VED " << em->name() << " open via commu " << communication << endl;
    if (start_reader()==0)
      {
      if (create_dataout()==-1)
        {
        stop_reader();
        delete em;
        close_commu();
        }
      else
        {
        test_readout();
        delete em;
        close_commu();
        }
      }
    else
      {
      delete em;
      close_commu();
      }
    }
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
}

/*****************************************************************************/
/*****************************************************************************/
