/*
 * anketest.cc
 * 
 * created: 14.08.1998 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
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

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: anketest.cc,v 1.4 2005/02/11 15:44:48 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_readargs* args;
STRING infile;
int ilit, irecsize, evnum, maxev,
    statclust, statsec, loop;

/*
 * Das Programm liest einen Cluster-Datenstrom und
 * wuehlt in den Events 'rum
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
void printheader(ostream& os)
{
os<<"Liest EMS cluster format und macht 'was damit."<<endl<<endl;
}
/*****************************************************************************/
void printstreams(ostream& os)
{
os<<endl;
os<<"input may be: "<<endl;
os<<"  - for stdin ;"<<endl;
os<<"  <filename> for regular file or tape;"<<endl;
os<<"      (if filename contains a colon use -iliterally)"<<endl;
os<<"  :<port#> or <host>:<port#> for tcp socket;"<<endl;
os<<"  :<name> for unix socket (passive);"<<endl;
os<<"  @:<name> for unix socket (active);"<<endl;
}
/*****************************************************************************/
int readargs()
{
args->helpinset(3, printheader);
args->helpinset(5, printstreams);

args->addoption("infile", 0, "-", "input file", "input");

args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("loop", "loop", false, "reopen input if exhausted", "loop");

args->addoption("irecsize", "irecsize", 65536, "maximum record size of input (tape only)",
    "irecsize");

args->addoption("evnum", "evnum", 0, "number of events to be used", "evnum");
args->addoption("maxev", "maxev", 0, "id of last event to be used", "maxev");

args->addoption("verbose", "verbose", false, "put out some comments", "verbose");
args->addoption("debug", "debug", false, "debug info", "debug");
args->addoption("statclust", "statclust", 1000, "print statistics after n clusters",
    "statclust");
args->addoption("statsec", "statsec", 60, "print statistics after n seconds",
    "statsec");

if (args->interpret(1)!=0) return(-1);
ilit=args->getboolopt("ilit");
infile=args->getstringopt("infile");
loop=args->getboolopt("loop");

irecsize=args->getintopt("irecsize");
debuglevel::debug_level=args->getboolopt("debug");
debuglevel::verbose_level=debuglevel::debug_level||args->getboolopt("verbose");
statclust=args->getintopt("statclust");
statsec=args->getintopt("statsec");
evnum=args->getintopt("evnum");
maxev=args->getintopt("maxev");
return(0);
}
/*****************************************************************************/
void use_event(int* ev)
{
}
/*****************************************************************************/
void do_convert(Clusterreader& reader)
{
if (debuglevel::trace_level) cerr<<"do_convert called"<<endl;
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
      use_event(event);
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
/*****************************************************************************/
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
  cerr<<"vor new reader"<<endl;
  reader=new Clusterreader(infile, ilit);
  cerr<<"nach new reader"<<endl;
  if (reader->input_status()!=Clusterreader::input_ok)
    {
    cerr<<"reader->input_status()="<<(int)reader->input_status()<<endl;
    delete reader;
    return 1;
    }
  reader->recsize(irecsize);
    cerr<<reader->iopath()<<endl;
  reader->statist().setup(statsec, statclust);
  if (debuglevel::verbose_level)
    {
    cerr<<reader->iopath()<<endl;
    }
  do
    {
    try
      {
      int res=0;
      do_convert(*reader);
      if (res!=0) loop=0;
      }
    catch (C_error* e)
      {
      cerr<<"caught in main: "<<(*e)<<endl;
      delete e;
      }
    if (loop && (reader->input_status()==Clusterreader::input_exhausted))
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
