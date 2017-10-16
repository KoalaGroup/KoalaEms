/*
 * tapescan.cc
 * 
 * created: 11.04.95 PW
 * 10.07.1998 PW: adapted for cluster (ANKE)
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <readargs.hxx>
#include <events.hxx>
#include "compat.h"
#include "versions.hxx"

VERSION("Jul 10 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: tapescan.cc,v 1.12 2004/11/26 14:40:21 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int debug;
int ask;
int single;
int fast;
STRING headline;
int buffered;
STRING infile;
int cluster, oldtext;
int maxrec, maxirec;
int ilit, olit;
int runnr;

class C_filevars
  {
  public:
    C_filevars():start_found(0), stop_found(0), pre_events(0),
        post_events(0), num_events(0) {}
    int start_found;
    int stop_found;
    int pre_events;
    int post_events;
    int num_events;
    void operator++(int);
  };

extern void scan_tape_with_cluster();

/*****************************************************************************/
void C_filevars::operator ++(int)
{
num_events++;
if (!start_found) pre_events++;
if (stop_found) post_events++;
}
/*****************************************************************************/
static int readargs()
{
args->addoption("verbose", "verbose", 0, "verbose");
args->addoption("debug", "debug", false, "debug");
args->addoption("ask", "ask", false, "ask for next file");
args->addoption("single", "single", false, "only one file");
args->addoption("headline", "headline", "", "headline for protocol", "headline");
args->addoption("no_tape", "nt", false, "input not from tape");
args->addoption("fast", "fast", false, "fast");
args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
args->addoption("olit", "oliterally", false,
    "use name of output file literally", "output file literally");
args->addoption("cluster", "cluster", true, "use cluster format");
args->addoption("oldtext", "oldtext", false, "old text format");
args->addoption("infile", 0, "/dev/ntape/tape0", "input file", "input");
args->addoption("runnr", "runnr", false, "print run number and rewind");
args->hints(C_readargs::exclusive, "ask", "single", 0);
args->hints(C_readargs::exclusive, "fast", "no_tape", 0);
//args->hints(C_readargs::implicit, "debug", "verbose", 0);

if (args->interpret(1)!=0) return(-1);

verbose=args->getintopt("verbose");
debug=args->getboolopt("debug");
ask=args->getboolopt("ask");
fast=args->getboolopt("fast");
cluster=args->getboolopt("cluster");
oldtext=args->getboolopt("oldtext");
single=args->getboolopt("single");
headline=args->getstringopt("headline");
buffered=(args->getboolopt("no_tape"));
infile=args->getstringopt("infile");
maxirec=args->getintopt("maxrec")/4;
maxrec=maxirec*4;
ilit=args->getboolopt("ilit");
olit=args->getboolopt("olit");
runnr=args->getboolopt("runnr");
return(0);
}
/*****************************************************************************/
static void printwatzstring(int* p, int size)
{
union
  {
  char s[256];
  int i[64];
  } uni;
int il=(size+3)/4;

int i;
for (i=0; i<il; i++) uni.i[i]=p[i];
uni.s[size]=0;
cout << uni.s << endl;
}
/*****************************************************************************/
static void do_header(C_eventp& event, C_filevars& vars)
{
switch (event[4])
  {
  case 1:
    {
    if (vars.start_found)
      {
      vars.stop_found=1;
      cout << "trailer:" << endl;
      }
    else if (vars.stop_found)
      {
      cout << "missing filemark?" << endl;
      cout << "super trailer:" << endl;
      }
    else
      {
      vars.start_found=1;
      cout << "header:";
      if (vars.pre_events) cout << " (" << vars.pre_events
        << " events before header)";
      cout << endl;
      }
    if (verbose) cout << "    main header" << endl;
    int run=event[7];
    cout << "run " << run << endl;
    printwatzstring((int*)(event+8), 28);
    if (verbose)
      {
      printwatzstring((int*)(event+15), 80);
      printwatzstring((int*)(event+35), 80);
      }
    }
    break;
  case 2: if (verbose) cout << "    hardware config" << endl; break;
  case 3: if (verbose) cout << "    module types" << endl; break;
  case 4: if (verbose) cout << "    synchronisation setup" << endl; break;
  case 5:
    if (verbose)
      {
      cout << "    is setup: ";
      switch (event[6])
        {
        case 1: cout << "fera" << endl; break;
        case 2: cout << "fastbus multi block" << endl; break;
        case 3: cout << "struck dl350" << endl; break;
        case 4: cout << "lecroy tdc2277" << endl; break;
        case 5: cout << "fastbus lecroy" << endl; break;
        case 6: cout << "camac scaler" << endl; break;
        case 7: cout << "zel discriminator" << endl; break;
        case 8: cout << "fastbus kinetic" << endl; break;
        case 9: cout << "drams" << endl; break;
        case 10: cout << "lecroy tdc2228" << endl; break;
        case 11: cout << "multi purpose" << endl; break;
        default: cout << "unknown type: " << event[6] << endl; break;
        }
      }
    break;
  case 6: if (verbose) cout << "    user record" << endl; break;
  default:
    if (verbose ) cout << "    unknown record: " << event[4] << endl;
    break;
  }
}

/*****************************************************************************/
static void scan_file(C_evinput* evin, int& fnum)
{
C_eventp event;
C_filevars vars;
int wrote_line=0;
int first, evnum=0;
int event_found=0;
int records_found=0;
int mismatches=0;

if (debug) cerr << "scan_file()" << endl;
first=0;
try
  {
  while ((*evin) >> event)
    {
    if (!wrote_line)
      {
      cout <<
"------------------------------------------------------------------------------"
          << endl;
      if (!single) cout << "File " << ++fnum << endl;
      wrote_line=1;
      }
    records_found++;
    if (event.evnr()==0)
      do_header(event, vars);
    else
      {
      if (!event_found)
        {
        first=event.evnr();
        event_found=1;
//      cout << "first event: " << first << endl;
        }
      else if (event.evnr()!=evnum+1)
        {
        if (mismatches++<=10)
          {
          if (event.evnr()==evnum+1)
            {
            cout << "event " << evnum << " missing" << endl;
            }
          else if (event.evnr()>evnum)
            {
            cout << "events from " << evnum << " to " << event.evnr()-1
                << " missing" << endl;
            }
          else
            {
            cout << "jump from event " << evnum-1 << " to " << event.evnr()
                << endl;
            }
          if (!verbose && (mismatches==10))
            cout << "further mismatches suppressed" << endl;
          }
        }
      vars++;
      evnum=event.evnr();
      }
    }
  if (records_found)
    {
    if (event_found)
      {
      cout << "first event     : " << first << endl;
      cout << "last event      : " << evnum << endl;
      cout << "number of events: " << vars.num_events;
      if (vars.post_events) cout << " (" << vars.post_events
          << " events after trailer)";
      cout << endl;
      }
    else
      cout << "no event found" << endl;
    if (mismatches>10) cout << mismatches << " mismatches" << endl;
    if (!vars.start_found) cout << "no header found" << endl;
    if (!vars.stop_found) cout << "no trailer found" << endl;
    }
  if (evin->is_tape())
    {
    C_evtinput* in=(C_evtinput*)evin;
    if (!in->fatal() && in->filemark()) cout << "filemark" << endl;
    }
  }
catch (C_error* error)
  {
  cout << (*error) << endl;
  delete error;
  }
}
/*****************************************************************************/
static void scan_fast(C_evtinput* evin, int& fnum)
{
C_eventp event;
C_filevars vars;
int wrote_line=0;
int first, evnum=0;
int event_found=0;
int records_found=0;
int numheaders;

if (debug) cerr << "scan_fast()" << endl;
first=0;
numheaders=0;
try
  {
  while ((!(event_found && (numheaders>0))) && (*evin) >> event)
    {
    if (!wrote_line)
      {
      cout <<
"------------------------------------------------------------------------------"
          << endl;
      if (!single) cout << "File " << ++fnum << endl;
      wrote_line=1;
      }
    records_found++;
    if (event.evnr()==0)
      {
      if (debug) cout << "header found" << endl;
      do_header(event, vars);
      numheaders++;
      }
    else
      {
      if (debug) cout << "event found" << endl;
      first=event.evnr();
      event_found=1;
      }
    }
  if (debug)
    {
    if (event_found)
      cerr << "first event: " << first << endl;
    else
      cerr << "no event found" << endl;
    cerr << numheaders << " headers found" << endl;
    }
  if (event_found)
    {
    if (debug) cerr << "windfile(1)" << endl;
    evin->windfile(1);
    if (debug) cerr << "windrecord(" << (-(numheaders+2)) << ")" << endl;
    try
      {
      evin->windrecord(-1);
      }
    catch (C_error* error)
      {
//    cerr << (*error) << " (ignored)" << endl;
      delete error;
      }
    evin->windrecord(-numheaders-2);
    while ((*evin) >> event)
      {
      if (event.evnr()==0)
        do_header(event, vars);
      else
        {
        if (debug) cerr << "event " << event.evnr() << endl;
        evnum=event.evnr();
        }
      }
    cout << "first event     : " << first << endl;
    cout << "last event      : " << evnum << endl;
    }
  else if (numheaders>0)
    {
    cout << "no event found" << endl;
    if (!vars.start_found) cout << "no header found" << endl;
    if (!vars.stop_found) cout << "no trailer found" << endl;
    }
  if (!evin->fatal() && evin->filemark()) cout << "filemark" << endl;
  }
catch (C_error* error)
  {
  cout << (*error) << endl;
  delete error;
  }
}
/*****************************************************************************/
static void scan_tape()
{
C_evinput* evin;

try
  {
  if (debug) cerr << "try to open" << endl;
  if (buffered)
    {
    if (debug) cerr << "open buffered" << endl;
    evin=new C_evfinput(infile, maxirec);
    }
  else
    {
    if (debug) cerr << "open tape" << endl;
    evin=new C_evtinput(infile, maxirec);
    }
  if (debug) cerr << "open ok" << endl;
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return;
  }

int fnum=0;
if (single)
  if (fast)
    scan_fast((C_evtinput*)evin, fnum);
  else
    scan_file(evin, fnum);
else
  {
  while (!evin->fatal())
    {
    if (fast)
      scan_fast((C_evtinput*)evin, fnum);
    else
      scan_file(evin, fnum);
    evin->reset();
    if (ask)
      {
      int dummy;
      cerr << "press enter for next file! " << flush;
      cin >> dummy;
      }
    }
  }

delete evin;
}
/*****************************************************************************/
int
main(int argc, char* argv[])
{
    args=new C_readargs(argc, argv);
    if (readargs()!=0) return(0);

    if (cluster) {
        scan_tape_with_cluster();
    } else {
        scan_tape();
    }

    return(0);
}
/*****************************************************************************/
/*****************************************************************************/
