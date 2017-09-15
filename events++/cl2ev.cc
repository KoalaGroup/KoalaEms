/*
 * cl2ev.cc
 * 
 * created: 25.05.1998 PW
 */

#include "cl2ev.hxx"

#include <iostream.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>
#include <errors.hxx>
#include "memory.hxx"
#include "swap_int.h"
#include <versions.hxx>
#include <debuglevel.hxx>

VERSION("May 25 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: cl2ev.cc,v 1.2 2005/02/11 15:44:50 wuestner Exp $")

/*
 * Die Klasse C_cluster2event dient der Konversion eines Cluster-Datenstromes in
 * einen reinen Eventstrom.
 * Es gibt vier Moeglichkeiten:
 * 
 * (default)
 *   verlustfrei mit unbegrenztem Speicher, bricht bei Speichermangel ab
 * 
 * -maxmem #
 *   verlustfrei mit begrenztem Speicher, Datenquelle muss seek erlauben
 * 
 * -maxmem # -sloppy
 *   verlustbehaftet synchron, Verlust tritt bei Speichermangel auf
 * 
 * -asynchron [-maxmem #] (impliziert -sloppy)
 *   verlustbehaftet asynchron, Verlust tritt bei Speichermangel und
 *     blockender Datensenke auf
 */

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

/*****************************************************************************/
///////////////////////////////////////////////////////////////////////////////
class C_evsender
  {
  protected:
    C_iopath& writer_;
    int* event_;
    int size_;
    int index_;
  public:
    C_evsender(C_iopath& writer):writer_(writer), event_(0) {}
    ~C_evsender() {delete[] event_;}
  public:
    int pending() const {return event_!=0;}
    void event(int* event);
    void send();
  };
//.............................................................................
void C_evsender::event(int* event)
{
if (debuglevel::trace_level) cerr<<"C_evsender::event called"<<endl;
if (event_) throw new C_program_error("event_!=0 in C_evsender");
event_=event;
size_=(event_[0]+1)*sizeof(int); index_=0;}
//.............................................................................
void C_evsender::send(void)
{
if (debuglevel::trace_level) cerr<<"C_evsender::send called"<<endl;
int dort=writer_.write_noblock((char*)event_+index_, size_-index_);
index_+=dort;
if (index_==size_)
  {
  delete[] event_;
  event_=0;
  }
}
///////////////////////////////////////////////////////////////////////////////
C_cluster2event::C_cluster2event(int argc, char** argv)
:argc(argc), argv(argv)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::C_cluster2event called"<<endl;
}
///////////////////////////////////////////////////////////////////////////////
C_cluster2event::~C_cluster2event()
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::~C_cluster2event called"<<endl;
}
///////////////////////////////////////////////////////////////////////////////
int C_cluster2event::run()
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::run called"<<endl;
int res=0;
setlocale(LC_TIME, "");
args=0;

C_clusterreader* reader=0;
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);
  reader=new C_clusterreader(infile, ilit);
  if (reader->input_status()!=C_clusterreader::input_ok)
    {
    delete reader;
    return 1;
    }
  reader->maxmem(maxmem);
  reader->setup_statist(statclust, statsec);
  C_iopath writer(outfile, C_iopath::iodir_output, olit);
  if (writer.typ()==C_iopath::iotype_none)
    {
    delete reader;
    return 1;
    }
  if (debuglevel::verbose_level)
    {
    cerr<<reader->iopath()<<endl;
    cerr<<writer<<endl;
    }

  do
    {
    try
      {
      int res=0;
      if (async)
        res=do_convert_async(*reader, writer);
      else if (sloppy)
        do_convert_sloppy(*reader, writer);
      else if (maxmem>0)
        do_convert_complete_with_mem(*reader, writer);
      else
        do_convert_complete(*reader, writer);
      if (res!=0) loop=0;
      }
    catch (C_error* e)
      {
      cerr<<"caught in main: "<<(*e)<<endl;
      delete e;
      }
    if (loop && (reader->input_status()==C_clusterreader::input_exhausted))
      {
      cerr<<"restart reader... "<<flush;
      reader->restart();
      if (reader->input_status()!=C_clusterreader::input_ok)
        loop=0;
      else
        cerr<<"ok"<<endl;
      }
    }
  while (loop);
  }
catch (C_error* e)
  {
  cerr<<(*e)<<endl;
  delete e;
  res=2;
  }
delete reader;
delete args;
return res;
}
///////////////////////////////////////////////////////////////////////////////
int C_cluster2event::readargs(void)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::readargs called"<<endl;
args->helpinset(3, printheader);
args->helpinset(5, printstreams);

args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");

args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("olit", "oliterally", false,
    "use name of output file literally", "output file literally");
args->addoption("loop", "loop", false, "reopen input if exhausted", "loop");

args->addoption("recsize", "recsize", 65536, "maximum record size (tape only)",
    "recsize");
args->addoption("swap", "swap", false, "swap output", "swap");

args->addoption("sloppy", "sloppy", false, "sloppy", "sloppy");
args->addoption("async", "asynchron", false, "asynchron", "asynchron");

args->addoption("maxmem", "maxmem", 0, "maximum memory usage in KByte", "maxmem");
args->addoption("evnum", "evnum", 0, "number of events to be copied", "evnum");
args->addoption("maxev", "maxev", 0, "id of last event to be copied", "maxev");

args->addoption("verbose", "verbose", false, "put out some comments", "verbose");
args->addoption("debug", "debug", false, "debug info", "debug");
args->addoption("statclust", "statclust", 1000, "print statistics after n clusters",
    "statclust");
args->addoption("statsec", "statsec", 60, "print statistics after n seconds",
    "statsec");

args->hints(C_readargs::implicit, "async", "sloppy", 0);
args->hints(C_readargs::demands, "async", "maxmem", 0);

if (args->interpret(1)!=0) return(-1);
ilit=args->getboolopt("ilit");
olit=args->getboolopt("olit");
infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
loop=args->getboolopt("loop");

recsize=args->getintopt("recsize");
swap_out=args->getboolopt("swap");
sloppy=args->getboolopt("sloppy");
async=args->getboolopt("async");
debuglevel::debug_level=args->getboolopt("debug");
debuglevel::verbose_level=debuglevel::debug_level||args->getboolopt("verbose");
statclust=args->getintopt("statclust");
statsec=args->getintopt("statsec");
maxmem=args->getintopt("maxmem")*1024;
evnum=args->getintopt("evnum");
maxev=args->getintopt("maxev");
return(0);
}
///////////////////////////////////////////////////////////////////////////////
int C_cluster2event::do_convert_async(C_clusterreader& reader, C_iopath& writer)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::do_convert_async called"<<endl;
reader.sloppy(1);
int ok=1;
if (debuglevel::debug_level)
  {
  cerr<<"input is ";
  switch (reader.iopath().typ())
    {
    case C_iopath::iotype_none: cerr<<"none"; break;
    case C_iopath::iotype_tape: cerr<<"tape"; break;
    case C_iopath::iotype_file: cerr<<"file"; break;
    case C_iopath::iotype_socket: cerr<<"socket"; break;
    case C_iopath::iotype_fifo: cerr<<"fifo"; break;
    default: cerr<<"unknown ("<<(int)reader.iopath().typ()<<")"; break;
    }
  cerr<<endl;
  cerr<<"output is ";
  switch (writer.typ())
    {
    case C_iopath::iotype_none: cerr<<"none"; break;
    case C_iopath::iotype_tape: cerr<<"tape"; break;
    case C_iopath::iotype_file: cerr<<"file"; break;
    case C_iopath::iotype_socket: cerr<<"socket"; break;
    case C_iopath::iotype_fifo: cerr<<"fifo"; break;
    default: cerr<<"unknown ("<<(int)writer.typ()<<")"; break;
    }
  cerr<<endl;
  }

switch (reader.iopath().typ())
  {
  default:     // nobreak
  case C_iopath::iotype_none: // nobreak
  case C_iopath::iotype_tape: // nobreak
  case C_iopath::iotype_file:
    cerr<<"input does not support select system call."<<endl;
    ok=0;
    break;
  case C_iopath::iotype_socket: // nobreak
  case C_iopath::iotype_fifo:
    // ok
    break;
  }
switch (writer.typ())
  {
  default:     // nobreak
  case C_iopath::iotype_none: // nobreak
  case C_iopath::iotype_tape: // nobreak
  case C_iopath::iotype_file:
    cerr<<"output does not support nonblocking write."<<endl;
    ok=0;
    break;
  case C_iopath::iotype_socket: // nobreak
  case C_iopath::iotype_fifo:
    // ok
    break;
  }
if (!ok) return -1;
writer.noblock();

C_evsender sender(writer);

int rpath, wpath, numpath;
rpath=reader.iopath().path();
wpath=writer.path();
numpath=(rpath>wpath?rpath:wpath)+1;

int ccount=0, ecount=0, weiter=1;
while (weiter)
  {
  fd_set readfds, writefds, exceptfds;
  int res;
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&exceptfds);
  if (reader.input_status()==C_clusterreader::input_ok)
    {
    FD_SET(rpath, &readfds);
    FD_SET(rpath, &exceptfds);
    }
  else
    break; // bloeder hack
  FD_SET(wpath, &writefds);
  FD_SET(wpath, &exceptfds);
  struct timeval tv={300, 0};
  res=select(numpath, &readfds, &writefds, &exceptfds, &tv);
  if (res<0)
    {
    cerr<<"select: res="<<res<<"; errno="<<strerror(errno)<<endl;
    }
  else
    {
    if (FD_ISSET(rpath, &exceptfds)) cerr<<"exception for read"<<endl;
    if (FD_ISSET(wpath, &exceptfds)) cerr<<"exception for write"<<endl;
    if (FD_ISSET(rpath, &readfds))
      {
      reader.read_cluster(); // blocking read
      ccount++;
      }
    if (FD_ISSET(wpath, &writefds))
      {
      if (sender.pending())
        sender.send();
      else
        {
        int* event=reader.get_event();
        if (event)
          {
          ecount++;
          if (ecount%10000==0)
            {
            cerr<<"event="<<ecount<<" (id="<<event[1]<<"); mem="
                <<memory::used_mem<<endl;
            }
          sender.event(event);
          sender.send();
          }
        else
          {
          switch (reader.input_status())
            {
            case C_clusterreader::input_ok:
              cerr<<"input ok but no events"<<endl;
              break;
            case C_clusterreader::input_exhausted:
              cerr<<"input exhausted"<<endl;
              weiter=0;
              break;
            case C_clusterreader::input_error:
              cerr<<"input error"<<endl;
              weiter=0;
              break;
            default:
              cerr<<"unknown input status "<<(int)reader.input_status()<<endl;
              weiter=0;
              break;
            }
          } // if (event)
        } // if sender.pending()
      } // if FD_ISSET(wpath, ...)
    } // if (res<0)
  } // while
cerr<<ecount<<" events from "<<ccount<<" clusters processed"<<endl;
return 0;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster2event::do_convert_sloppy(C_clusterreader& reader,
    C_iopath& writer)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::do_convert_sloppy called"<<endl;
reader.sloppy(1);
cerr<<"sloppy modus not yet implemented"<<endl;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster2event::do_convert_complete_with_mem(C_clusterreader& reader,
    C_iopath& writer)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::do_convert_complete_with_mem called"<<endl;
reader.sloppy(1);
cerr<<"complete_with_mem modus not yet implemented"<<endl;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster2event::do_convert_complete(C_clusterreader& reader,
  C_iopath& writer)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::do_convert_complete called"<<endl;
reader.sloppy(0);
int count=0, weiter=1;
do
  {
  int* event;
  try
    {
    event=reader.get_event();
    }
  catch (C_error* e)
    {
    cerr<<(*e)<<endl;
    delete e;
    cerr<<count<<" events processed so far."<<endl;
    return;
    }

  if (event==(int*)-1)
    {
    cerr<<"got error from reader"<<endl;
    }
  else if (event==0)
    {
    // cerr<<"got no event from reader"<<endl;
    }
  else
    {
    if (debuglevel::trace_level) cerr<<"event "<<event[1]<<endl;
    if ((maxev==0) || (event[1]<=maxev))
      {
      write_event(event, writer);
      count++;
      }
    else
      weiter=0;
    delete[] event;
    if ((evnum!=0) && (count>=evnum)) weiter=0;
    }
  }
while (weiter);
cerr<<count<<" events processed"<<endl;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster2event::printheader(ostream& os)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::printheader called"<<endl;
os<<"Converts EMS cluster format into EMS legacy format."<<endl<<endl;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster2event::printstreams(ostream& os)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::printstreams called"<<endl;
os<<endl;
os<<"input and output may be: "<<endl;
os<<"  - for stdin/stdout ;"<<endl;
os<<"  <filename> for regular files or tape;"<<endl;
os<<"      (if filename contains a colon use -iliterally or -oliterally)"<<endl;
os<<"  :<port#> or <host>:<port#> for tcp sockets;"<<endl;
os<<"  :<name> for unix sockets (passive);"<<endl;
os<<"  @:<name> for unix sockets (active);"<<endl;
}
///////////////////////////////////////////////////////////////////////////////
void C_cluster2event::write_event(int* event, C_iopath& writer)
{
if (debuglevel::trace_level) cerr<<"C_cluster2event::write_event called"<<endl;
int size=event[0]+1;
if (swap_out)
  {
  for (int i=0; i<size; i++) event[i]=swap_int(event[i]);
  }
writer.write(event, size*sizeof(int));
}
///////////////////////////////////////////////////////////////////////////////
int run_main(int argc, char** argv)
{
int fehler;

C_cluster2event Main_obj(argc, argv);

fehler=Main_obj.run();

return fehler;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
