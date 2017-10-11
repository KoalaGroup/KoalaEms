/*
 * evselect.cc
 * 
 * created: 24.01.95 PW
 */

#include "config.h"
#include <readargs.hxx>
#include <iostream>
#include <sstream>
#include <string>
#include <cerrno>
#include <cstring>
#include <sys/ioctl.h>
#include <events.hxx>
#include <compat.h>
#include <versions.hxx>

VERSION("2014-07-14", __FILE__, __DATE__, __TIME__,
"$ZEL: evselect.cc,v 1.10 2014/07/14 16:18:17 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int numignore;
int numcopy;
int tapeinput;
int tapeoutput;
int copyonly;
int purge;
int usr_plus;
int usr_minus;
int usr_save;
int skip;
int maxevnum;
int maxevid;
int recsize;
string infile;
string outfile;

using namespace std;

/*****************************************************************************/
static int
readargs(void)
{
args->addoption("verbose", "v", false, "verbose");
args->addoption("ignore", "i", 0, "subevents to be ignored", "i_sub");
args->multiplicity("ignore", 0);
args->addoption("copy", "c", 0, "subevents to be copied", "c_sub");
args->multiplicity("copy", 0);
args->addoption("tape_in", "ti", false, "input is tape");
args->addoption("tape_out", "to", false, "output is tape");
args->addoption("buffered_in", "bi", false, "input is buffered",
    "buffered_input");
args->addoption("buffered_out", "bo", false, "output is buffered",
    "buffered_output");
args->addoption("swap", "swap", false, "swap words in output");
args->addoption("purge", "purge", false, "purge empty events");
args->addoption("skip", "skip", -1, "events to be skipped", "skip");
args->addoption("maxevnum", "evmax", -1, "maximum number of events to be stored",
    "events");
args->addoption("maxevid", "idmax", -1, "id of last event to be stored",
    "maxid");
args->addoption("maxexpect", "maxexp", 0, "maximum expected eventsize",
    "maxexp");
args->addoption("usrprefix", "usrprefix", "usr_",
    "prefix to be used for userrecords", "prefix");
args->addoption("usrsuffix", "usrsuffix", "",
    "suffix to be used for userrecords", "suffix");
args->addoption("usr+", "usr+", false,
    "read userrecords from files");
args->addoption("usr-", "usr-", false,
    "delete userrecords from eventstream");
args->addoption("usr_save", "usr_save", false,
    "save userrecords in files");
args->addoption("recsize", "recsize", 65536/4, "maximum record size", "recsize");
args->addoption("infile", 0, "-", "input file (- - for stdin)", "input");
args->addoption("outfile", 1, "-", "output file", "output");
if (args->interpret(1)!=0) return -1;

verbose=args->getboolopt("verbose");
numignore=args->multiplicity("ignore");
numcopy=args->multiplicity("copy");
if (numignore && numcopy)
  {
  cerr << "use either ignore or copy" << endl;
  return(-1);
  }
tapeinput=args->getboolopt("tape_in");
tapeoutput=args->getboolopt("tape_out");
infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");

const char* const deftape="/dev/rmt0h";
if (tapeinput && (infile=="-")) infile=deftape;
if (tapeoutput && (outfile=="-")) outfile=deftape;
if ((infile=="-") && (infile==outfile))
  {
  cerr << "input file and output file must be different" << endl;
  return(-1);
  }

purge=args->getboolopt("purge");
usr_plus=args->getboolopt("usr+");
usr_minus=args->getboolopt("usr-");
usr_save=args->getboolopt("usr_save");

skip=args->getintopt("skip");
maxevnum=args->getintopt("maxevnum");
maxevid=args->getintopt("maxevid");
recsize=args->getintopt("recsize");
copyonly=(numignore==0) && (numcopy==0) && !purge && !usr_plus && !usr_minus
    && !usr_save;

return(0);
}

/*****************************************************************************/
static void
read_usr_records(C_evoutput& evout)
{
const string prefix(args->getstringopt("usrprefix"));
const string suffix(args->getstringopt("usrsuffix"));
int idx, ok;
C_event event;
FILE* f;

clog << "read_usr_records: prefix=" << prefix << "; suffix=" << suffix << endl;
idx=0;
do
  {
  ostringstream str;

  str << prefix << setfill('0') << setw(4) << idx << suffix << ends;
  string st=str.str();
  const char* namep=st.c_str();
  clog << "suffix=" << suffix << endl;
  clog << "name=" << namep << endl;

  if ((f=fopen(namep, "r"))==0)
    {
    clog << "read_usr_records: open file " << namep << ": "<< strerror(errno)
        << endl;
    ok=0;
    }
  else
    {
    event.read_usr(f);
    evout << event;
    ok=1;
    }
  idx++;
  }
while (ok);
}

/*****************************************************************************/
static int
save_usr_record(C_eventp& event, int idx)
{
const string prefix(args->getstringopt("usrprefix"));
const string suffix(args->getstringopt("usrsuffix"));
char name[1024];
FILE* f;
int res;

ostringstream str;
str << prefix << setfill('0') << setw(4) << idx << suffix << ends;
string st=str.str();
const char* namep=st.c_str();
clog << "name=" << namep << endl;
if ((f=fopen(name, "w"))==0)
  {
  clog << "save_usr_record: open file " << namep << ": "<< strerror(errno)
      << endl;
  res=-1;
  }
else
  {
  event.write_usr(f);
  fclose(f);
  res=0;
  }
return res;
}

/*****************************************************************************/
static int
do_select()
{
C_evinput* evin;
C_evoutput* evout;
C_eventp inevent;
C_event outevent;
C_subeventp subevent;

try
  {
  if (tapeinput)
    evin=new C_evtinput(infile, recsize);
  else if (args->getboolopt("buffered_in"))
    evin=new C_evfinput(infile, recsize);
  else
    evin=new C_evpinput(infile, recsize);
  evin->setmaxexpect(args->getintopt("maxexpect"));
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return(-1);
  }
try
  {
  if (tapeoutput)
    evout=new C_evtoutput(outfile, recsize);
  else if (args->getboolopt("buffered_out"))
    evout=new C_evfoutput(outfile, recsize);
  else
    evout=new C_evpoutput(outfile, recsize);
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  delete evin;
  return(-1);
  }

if (numignore)
  {
  int i;
  for (i=0; i<numignore; i++)
      inevent >> setignore(args->getintopt("ignore", i));
  }

if (numcopy)
  {
  int i;
  for (i=0; i<numcopy; i++)
      inevent >> setcopy(args->getintopt("copy", i));
  }

if (usr_plus) read_usr_records(*evout);

try
  {
  int saved=0;
  int usrnum=0;
  int weiter=1;
  int lastevent=0, wrote=0;
  evout->swap(args->getboolopt("swap"));
  while (weiter && ((*evin) >> inevent))
    {
    if (inevent.evnr()==0)
      {
      if (usr_save) if (save_usr_record(inevent, usrnum)!=0) weiter=0;
      if (usr_minus)
        clog << "userrecord " << usrnum << " suppressed" << endl;
      else
        (*evout) << flush << inevent << flush;
      usrnum++;
      }
    else
      {
      if (verbose)
        {
        if (inevent.evnr()!=lastevent+1)
          {
          if (!wrote) clog << "... " << lastevent << endl;
          clog << inevent.evnr() << endl;
          wrote=1;
          }
        else
          wrote=0;
        lastevent=inevent.evnr();
        }
      if ((inevent.evnr()%100000)==0) clog << "event " << inevent.evnr() << endl;
      if ((maxevid>0) && (inevent.evnr()>maxevid))
        weiter=0;
      else
        {
        outevent.clear();
        outevent.evnr(inevent.evnr());
        outevent.trignr(inevent.trignr());
        while (inevent >> subevent) outevent << subevent;
        if (!outevent.empty() || !purge)
          {
          (*evout) << outevent;
          if ((++saved>maxevnum) && (maxevnum>0)) weiter=0;
          }
        }
      }
    }
  clog << saved << " events stored" << "; last event: " << outevent.levnr()
      << endl;
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  }

delete evin;
delete evout;
return 0;
}

/*****************************************************************************/
static int
do_copy(void)
{
C_evinput* evin;
C_evoutput* evout;
C_eventp event;

try
  {
  if (tapeinput)
    evin=new C_evtinput(infile, recsize);
  else if (args->getboolopt("buffered_in"))
    evin=new C_evfinput(infile, recsize);
  else
    evin=new C_evpinput(infile, recsize);
  evin->setmaxexpect(args->getintopt("maxexpect"));
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return(-1);
  }
try
  {
  if (tapeoutput)
    evout=new C_evtoutput(outfile, recsize);
  else if (args->getboolopt("buffered_out"))
    evout=new C_evfoutput(outfile, recsize);
  else
    evout=new C_evpoutput(outfile, recsize);
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  delete evin;
  return(-1);
  }

try
  {
  int saved=0, savedheads=0;
  int weiter=1;
  int skipped=0;
  evout->swap(args->getboolopt("swap"));
  while (weiter && ((*evin) >> event))
    {
    if ((maxevid>0) && (event.evnr()>maxevid))
      weiter=0;
    else
      {
      if (event.evnr()==0)
        {
        (*evout) << flush << event << flush;
        savedheads++;
        }
      else
        {
        if (skip<=0)
          {
          (*evout) << event;
          saved++;
          }
        else
          {
          if (skipped<skip)
            skipped++;
          else
            {
            (*evout) << event;
            saved++;
            }
          }
        }
      if ((maxevnum>0) && (saved>=maxevnum)) weiter=0;
      }
    }
  clog << saved << " events stored" << "; last event: " << event.levnr()
      << endl;
  clog<<savedheads<<" header records stored"<<endl;
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  }

delete evin;
delete evout;
return 0;
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

if (verbose)
  {
  if (tapeinput) clog << "input from tape" << endl;
  if (tapeoutput) clog << "output to tape" << endl;
  clog << "name of input file: " << infile << endl;
  clog << "name of output file: " << outfile << endl;
  clog << endl;
  }

if (copyonly)
  do_copy();
else
  do_select();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
