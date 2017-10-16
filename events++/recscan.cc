/******************************************************************************
*                                                                             *
* recscan.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 18.04.95                                                           *
* last changed: 20.09.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <events.hxx>
#include <inbuf.hxx>
#include <compat.h>
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: recscan.cc,v 1.6 2005/02/11 15:45:11 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int debug;
int fast;
int buffered;
STRING infile;
STRING suffix;
STRING prefix;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "verbose", false, "verbose");
args->addoption("debug", "debug", false, "debug");
args->addoption("no_tape", "nt", false, "input not from tape");
args->addoption("fast", "fast", false, "fast");
args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");
args->addoption("usrprefix", "usrprefix", "",
    "prefix to be used for userrecords", "prefix");
args->addoption("usrsuffix", "usrsuffix", "",
    "suffix to be used for userrecords", "suffix");
args->hints(C_readargs::exclusive, "fast", "no_tape", 0);

if (args->interpret(1)!=0) return(-1);

verbose=args->getboolopt("verbose");
debug=args->getboolopt("debug");
fast=args->getboolopt("fast");
buffered=args->getboolopt("no_tape");
infile=args->getstringopt("infile");
suffix=args->getstringopt("usrsuffix");
prefix=args->getstringopt("usrprefix");

return(0);
}

/*****************************************************************************/

void printwatzstring(C_inbuf& in, int size, ofstream& f)
{
int* r=new int[size+1];
int i;
for (i=0; i<size; i++) r[i]=in.getint();
r[size]=0;
f << (char*)r;
delete[] r;
}

/*****************************************************************************/

void printwatzstring(C_inbuf& in, ofstream& f)
{
printwatzstring(in, in.getint(), f);
}

/*****************************************************************************/

void printwatzfile(C_inbuf& in, ofstream& f)
{
int recs=in.getint();
int i;
for (i=0; i<recs; i++)
  {
  printwatzstring(in, f);
  f << endl;
  }
}

/*****************************************************************************/

void header_1(C_inbuf& in, int idx, ofstream& f)
{
//cout << "dummy" << endl;
if (idx!=1)
  {
  cout << "main header: idx!=1 nicht zulaessig" << endl;
  return;
  }
in++; // is_type ueberspringen
f << "run " << in.getint() << endl;
printwatzstring(in, (28+3)/4, f); f << endl;
printwatzstring(in, (80+3)/4, f); f << endl;
printwatzstring(in, (80+3)/4, f); f << endl;
}

/*****************************************************************************/

void header_2(C_inbuf& in, int idx, ofstream& f)
{
in+=2; // is_type und run_nr ueberspringen
printwatzfile(in, f);
}

/*****************************************************************************/

void header_3(C_inbuf& in, int idx, ofstream& f)
{
in+=2; // is_type und run_nr ueberspringen
printwatzfile(in, f);
}

/*****************************************************************************/

void header_4(C_inbuf& in, int idx, ofstream& f)
{
in+=2; // is_type und run_nr ueberspringen
printwatzfile(in, f);
}

/*****************************************************************************/

void header_5(C_inbuf& in, int idx, ofstream& f)
{
const char* isnames[]=
  {
  "fera",
  "fastbus multi block",
  "struck dl350",
  "lecroy tdc2277",
  "fastbus lecroy",
  "camac scaler",
  "zel discriminator",
  "fastbus kinetic",
  "drams",
  "lecroy tdc2228",
  "multi purpose",
  };

int is_type=in.getint()-1;
in++; // run_nr ueberspringen

if ((unsigned int)is_type<12)
  {
  f << "! IS type: " << isnames[is_type] << endl;
  f << "! =============================" << endl;
  }
else
  {
  f << "! IS type unknown: " << is_type+1 << endl;
  f << "! =============================" << endl;
  }
printwatzfile(in, f);
}

/*****************************************************************************/

void header_6(C_inbuf& in, int idx, ofstream& f)
{
in+=2; // is_type und run_nr ueberspringen
printwatzfile(in, f);
}

/*****************************************************************************/

void extract_header(C_evinput* evin, C_eventp& event, int& recnum)
{
const char* recnames[]=
  {
  "main_head",
  "hw_config",
  "mod_types",
  "sync_save",
  "isys_save",
  "user_save",
  };

OSSTREAM s;
int rec_type=event[4]-1;
if (((unsigned int)rec_type>5) || (suffix!=""))
  {
  s.fill('0');
  s << prefix << setw(2) << recnum << suffix << ends;
  }
else
  {
  s.fill('0');
  s << prefix << setw(2) << recnum << "_" << recnames[rec_type] << ends;
  }
STRING name=s.str();
ofstream f(name.c_str());
if (!f.good())
  {
  cout << "error opening file " << name << endl;
  return;
  }

int expected_type=0;
int expected_num =0;
int expected_idx =0;

int need_more=1;

while (need_more)
  {
  C_inbuf in(event.buffer());
  int usr_size, type, subtype, num, idx;

  in+=2; // ev_nr und trig_nr ueberspringen
  in >> usr_size >> type >> subtype;
  num      =(subtype>>16) & 0xffff;
  idx      = subtype      & 0xffff;

  if (expected_type)
    {
    if ((expected_type!=type) || (expected_num!=num) || (expected_idx!=idx))
      {
      cout << "da ist 'was falsch" << endl;
      return;
      }
    expected_type=type;
    expected_num=num;
    expected_idx=idx+1;
    }
  need_more=(num>idx);

  switch (type)
    {
    case 1: header_1(in, idx, f); break;
    case 2: header_2(in, idx, f); break;
    case 3: header_3(in, idx, f); break;
    case 4: header_4(in, idx, f); break;
    case 5: header_5(in, idx, f); break;
    case 6: header_6(in, idx, f); break;
    default:
      cout << "unknown record: " << type << endl;
      break;
    }

  if (need_more)
    {
    while (((*evin) >> event) && (event.evnr()!=0));
    if (!*evin)
      {
      cout << "records fehlen" << endl;
      return;
      }
    else
      recnum++;
    }
  }
}

/*****************************************************************************/

void scan_slow(C_evinput* evin)
{
C_eventp event;
int num=-1;

if (debug) clog << "scan_slow()" << endl;
try
  {
  while ((*evin) >> event)
    {
    if (event.evnr()==0)
      {
      num++;
      extract_header(evin, event, num);
      cout << "***" << endl;
      }
    else
      {
      cout << "event " << event.evnr() << endl;
      return;
      }
    }
  }
catch (C_error* error)
  {
  cout << (*error) << endl;
  delete error;
  }
}

/*****************************************************************************/

void scan_fast(C_evtinput* evin)
{
/*
C_eventp event;
C_headervars vars;
int wrote_line=0;
int first, last, evnum, numevents=0;
int event_found=0;
int records_found=0;
int numheaders;

if (debug) clog << "scan_fast()" << endl;
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
      clog << "first event: " << first << endl;
    else
      clog << "no event found" << endl;
    clog << numheaders << " headers found" << endl;
    }
  if (event_found)
    {
    if (debug) clog << "windfile(1)" << endl;
    evin->windfile(1);
    if (debug) clog << "windrecord(" << (-(numheaders+2)) << ")" << endl;
    try
      {
      evin->windrecord(-1);
      }
    catch (C_error* error)
      {
//    clog << (*error) << " (ignored)" << endl;
      delete error;
      }
    evin->windrecord(-numheaders-2);
    while ((*evin) >> event)
      {
      if (event.evnr()==0)
        do_header(event, vars);
      else
        {
        if (debug) clog << "event " << event.evnr() << endl;
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
  if (protocol) clog << (*error) << endl;
  cout << (*error) << endl;
  delete error;
  }
*/
}

/*****************************************************************************/

void scan_file()
{
C_evinput* evin;

try
  {
  if (debug) clog << "try to open" << endl;
  if (buffered)
    {
    if (debug) clog << "open buffered" << endl;
    evin=new C_evfinput(infile, 65536/4);
    }
  else
    {
    if (debug) clog << "open tape" << endl;
    evin=new C_evtinput(infile, 65536/4);
    }
  if (debug) clog << "open ok" << endl;
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return;
  }

if (fast)
  scan_fast((C_evtinput*)evin);
else
  scan_slow(evin);

delete evin;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

scan_file();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
