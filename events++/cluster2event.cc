/*
 * cluster2event.cc
 * 
 * created: 15.03.1998 PW
 * 29.07.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <versions.hxx>

#include <locale.h>
#include <readargs.hxx>
#include <errors.hxx>
#include "clusterreader.hxx"
#include "memory.hxx"
#include "swap_int.h"
#include <debuglevel.hxx>

VERSION("2014-07-14", __FILE__, __DATE__, __TIME__,
"$ZEL: cluster2event.cc,v 1.16 2014/07/14 16:18:17 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_readargs* args;
string infile, outfile;
int ilit, olit;
int swap_out, irecsize, orecsize, async, maxmem, evnum, maxev,
    statclust, statsec, loop, lowdelay, save_files;

/*
 * Das Programm dient der Konversion eines Cluster-Datenstromes in einen
 * reinen Eventstrom.
 * Es gibt drei Moeglichkeiten:
 * 
 * (default)
 *   verlustfrei mit unbegrenztem Speicher, bricht bei Speichermangel ab
 * 
 * -maxmem #
 *   verlustfrei mit begrenztem Speicher, Datenquelle muss seek erlauben
 * 
 * -asynchron [-maxmem #]
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
static void
printheader(ostream& os)
{
os<<"Converts EMS cluster format into EMS legacy format."<<endl<<endl;
}

/*****************************************************************************/
static void
printstreams(ostream& os)
{
os<<endl;
os<<"input and output may be: "<<endl;
os<<"  - for stdin/stdout ;"<<endl;
os<<"  <filename> for regular files or tape;"<<endl;
os<<"      (if filename contains a colon use -iliterally or -oliterally)"<<endl;
os<<"  :<port#> or <host>:<port#> for tcp sockets;"<<endl;
os<<"  :<name> for unix sockets (passive);"<<endl;
os<<"  @:<name> for unix sockets (active);"<<endl;
}

/*****************************************************************************/
static int
readargs(void)
{
args->helpinset(3, printheader);
args->helpinset(5, printstreams);

args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");

args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("olit", "oliterally", false,
    "use name of output file literally", "output file literally");
args->addoption("loop", "loop", false, "reopen input if exhausted", "loop");

args->addoption("irecsize", "irecsize", 65536, "maximum record size of input (tape only)",
    "irecsize");
args->addoption("orecsize", "orecsize", 65536, "maximum record size for output",
    "orecsize");

args->addoption("swap", "swap", false, "swap output", "swap");

args->addoption("async", "asynchron", false, "asynchron", "asynchron");

args->addoption("maxmem", "maxmem", 0, "maximum memory usage in KByte", "maxmem");
args->addoption("lowdelay", "lowdelay", false, "minimize delay", "lowdelay");
args->addoption("evnum", "evnum", 0, "number of events to be copied", "evnum");
args->addoption("maxev", "maxev", 0, "id of last event to be copied", "maxev");

args->addoption("verbose", "verbose", false, "put out some comments", "verbose");
args->addoption("debug", "debug", false, "debug info", "debug");
args->addoption("statclust", "statclust", 1000, "print statistics after n clusters",
    "statclust");
args->addoption("statsec", "statsec", 60, "print statistics after n seconds",
    "statsec");
args->addoption("save_files", "save_files", false, "save haeder and trailer as files",
    "save");

args->hints(C_readargs::demands, "async", "maxmem", 0);
args->hints(C_readargs::demands, "maxmem", "async", 0);

if (args->interpret(1)!=0) return(-1);
ilit=args->getboolopt("ilit");
olit=args->getboolopt("olit");
infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
loop=args->getboolopt("loop");

irecsize=args->getintopt("irecsize");
orecsize=args->getintopt("orecsize");
swap_out=args->getboolopt("swap");
async=args->getboolopt("async");
debuglevel::debug_level=args->getboolopt("debug");
debuglevel::verbose_level=debuglevel::debug_level||args->getboolopt("verbose");
statclust=args->getintopt("statclust");
statsec=args->getintopt("statsec");
maxmem=args->getintopt("maxmem")*1024;
lowdelay=args->getboolopt("lowdelay");
evnum=args->getintopt("evnum");
maxev=args->getintopt("maxev");
save_files=args->getboolopt("save_files");
return(0);
}

/*****************************************************************************/

class C_sender
  {
  protected:
    C_iopath& writer_;
    int* event_;
    int size_;
    int index_;
  public:
    C_sender(C_iopath& writer):writer_(writer), event_(0) {}
    ~C_sender() {delete[] event_;}
  public:
    int pending() const {return event_!=0;}
    void event(int* event);
    void send();
  };
//.............................................................................
void C_sender::event(int* event)
{
if (event_) throw new C_program_error("event_!=0 in C_sender");
event_=event;
size_=(event_[0]+1)*sizeof(int); index_=0;
if (swap_out)
  {
  int size=event[0]+1;
  for (int i=0; i<size; i++) event[i]=swap_int(event[i]);
  }
}
//.............................................................................
void C_sender::send(void)
{
int dort=writer_.write_noblock(reinterpret_cast<char*>(event_)+index_,
        size_-index_);
index_+=dort;
if (index_==size_)
  {
  delete[] event_;
  event_=0;
  }
}
/*****************************************************************************/
#if 0
static void
got_events(C_cluster* cluster)
{
static int count=5;
if (count-->=0)
  {
  cerr<<(*cluster)<<endl;
  cluster->dump(cerr, 40);
  }
}
#endif
/*****************************************************************************/
#if 0
static void
got_ved_info(C_cluster* cluster)
{
cerr<<(*cluster)<<endl;
//cluster->dump(cerr, 20);
//int vednum=cluster->get_vednum();
//int isnum=cluster->get_isnum();
//int* is_ids=cluster->get_is_ids();
if (debuglevel::verbose_level)
  {
  cerr<<"number of VEDs="<<cluster->get_vednum();
  cerr<<"; number of IS="<<cluster->get_isnum();
  cerr<<" (";
  for (int i=0; i<cluster->get_isnum(); i++)
    {
    cerr<<cluster->get_is_ids()[i];
    if (i+1<cluster->get_isnum()) cerr<<", ";
    }
  cerr<<")"<<endl;
  }
}
#endif
/*****************************************************************************/
#if 0
static void
got_text(C_cluster* cluster)
{
cerr<<(*cluster)<<endl;
cluster->dump(cerr, 20);
}
#endif
/*****************************************************************************/
#if 0
static void
got_no_more_data(C_cluster* cluster)
{
cerr<<(*cluster)<<endl;
cluster->dump(cerr, 20);
}
#endif
/*****************************************************************************/
static void
write_event(int* event, C_iopath& writer)
{
int size=event[0]+1;
if (swap_out)
  {
  for (int i=0; i<size; i++) event[i]=swap_int(event[i]);
  }
//writer.write_buffered(event, size*sizeof(int), 0);
writer.write(event, size*sizeof(int), 0);
}
/*****************************************************************************/
static void
do_convert_complete(Clusterreader& reader, C_iopath& writer)
{
if (debuglevel::trace_level) cerr<<"do_convert_complete called"<<endl;
writer.wbuffersize(orecsize);
int count=0, weiter=1;
do
  {
  int* event;
  try
    {
    event=reader.get_next_event();
    }
  catch (C_error* e)
    {
    cerr<<(*e)<<endl;
    delete e;
    cerr<<count<<" events processed so far."<<endl;
    return;
    }

  if (event==0)
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
writer.flush_buffer();
cerr<<count<<" events processed"<<endl;
}
/*****************************************************************************/
static int
do_convert_async(Clusterreader& reader, C_iopath& writer)
{
if (debuglevel::trace_level) cerr<<"do_convert_async called"<<endl;
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
    default:
        cerr<<"unknown ("<<static_cast<int>(reader.iopath().typ())<<")";
        break;
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
    default: cerr<<"unknown ("<<static_cast<int>(writer.typ())<<")"; break;
    }
  cerr<<endl;
  }

switch (reader.iopath().typ())
  {
  default:     // nobreak
  case C_iopath::iotype_none: // nobreak
  case C_iopath::iotype_tape: // nobreak
  case C_iopath::iotype_file:
    cerr<<"input ("<<reader.iopath()<<") does not support select system call."<<endl;
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
    cerr<<"output ("<<writer<<") does not support nonblocking write."<<endl;
    ok=0;
    break;
  case C_iopath::iotype_socket: // nobreak
  case C_iopath::iotype_fifo:
    // ok
    break;
  }
if (!ok) return -1;
writer.noblock();

C_sender sender(writer);

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
  if (reader.input_status()==Clusterreader::input_ok)
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
        int* event=reader.get_best_event(lowdelay);
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
            case Clusterreader::input_ok:
              //cerr<<"input ok but no events"<<endl;
              reader.read_cluster(); // blocking read
              ccount++;
              break;
            case Clusterreader::input_exhausted:
              cerr<<"input exhausted"<<endl;
              weiter=0;
              break;
            case Clusterreader::input_error:
              cerr<<"input error"<<endl;
              weiter=0;
              break;
            default:
              cerr<<"unknown input status "
                    <<static_cast<int>(reader.input_status())<<endl;
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
/*****************************************************************************/
int
main(int argc, char* argv[])
{
int res=0;
setlocale(LC_TIME, "");
args=0;
Clusterreader* reader=0;

try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);
  reader=new Clusterreader(infile, ilit);
  if (reader->input_status()!=Clusterreader::input_ok)
    {
    cerr<<"reader->input_status()="
            <<static_cast<int>(reader->input_status())<<endl;
    delete reader;
    return 1;
    }
  reader->maxmem(maxmem);
  reader->recsize(irecsize);
  reader->save_header(save_files);
  reader->statist().setup(statsec, statclust);
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
      else
        do_convert_complete(*reader, writer);
      if (res!=0) loop=0;
      }
    catch (C_error* e)
      {
      cerr<<"caught in main: "<<(*e)<<endl;
      delete e;
      }
    if (loop/* && (reader->input_status()==Clusterreader::input_exhausted)*/)
      {
      cerr<<"restart reader... "<<flush;
      reader->restart();
      if (reader->input_status()!=Clusterreader::input_ok)
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
/*****************************************************************************/
/*****************************************************************************/
