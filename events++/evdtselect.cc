/******************************************************************************
*                                                                             *
* evselect.cc                                                                 *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 24.01.95                                                           *
* last changed: 11.01.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <sys/ioctl.h>
#include <events.hxx>
#include <compat.h>
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: evdtselect.cc,v 1.5 2005/02/11 15:45:03 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int debug;
int tapeinput;
int tapeoutput;
int maxevnum;
int maxevid;
STRING infile;
STRING outfile;
int dt_is;
int mindt, maxdt, usemin, usemax, recsize;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("tape_in", "ti", false, "input is tape");
args->addoption("tape_out", "to", false, "output is tape");
args->addoption("buffered_in", "bi", false, "input is buffered",
    "buffered_input");
args->addoption("buffered_out", "bo", false, "output is buffered",
    "buffered_output");
args->addoption("swap", "swap", false, "swap words in output");
args->addoption("dt_is", "is", 0, "IS with deadtime", "IS");
args->addoption("maxevnum", "evmax", -1, "maximum number of events to be stored",
    "events");
args->addoption("maxevid", "idmax", -1, "id of last event to be stored",
    "maxid");
args->addoption("maxexpect", "maxexp", 0, "maximum expected eventsize",
    "maxexp");
args->addoption("mindt", "mindt", 0, "minimum deadtime (/100ns)", "time");
args->addoption("maxdt", "maxdt", 0, "maximum deadtime (/100ns)", "time");
args->addoption("recsize", "recsize", 65536/4, "record size", "recsize");
args->addoption("infile", 0, "-", "input file (- - for stdin)", "input");
args->addoption("outfile", 1, "-", "output file", "output");
args->hints(C_readargs::required, "dt_is", 0);
args->hints(C_readargs::implicit, "debug", "verbose", 0);

if (args->interpret(1)!=0) return -1;

debug=args->getboolopt("debug");
verbose=args->getboolopt("verbose");
tapeinput=args->getboolopt("tape_in");
tapeoutput=args->getboolopt("tape_out");
infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");

dt_is=args->getintopt("dt_is");
if (usemin=!args->isdefault("mindt")) mindt=args->getintopt("mindt");
if (usemax=!args->isdefault("maxdt")) maxdt=args->getintopt("maxdt");
maxevnum=args->getintopt("maxevnum");
maxevid=args->getintopt("maxevid");
recsize=args->getintopt("recsize");
if (verbose)
  {
  if (usemin)
    clog << "mindt="<<mindt<<endl;
  else
    clog << "no mindt" << endl;
  if (usemax)
    clog << "maxdt="<<maxdt<<endl;
  else
    clog << "no maxdt" << endl;
  }
return(0);
}

/*****************************************************************************/

int do_select()
{
C_evinput* evin;
C_evoutput* evout;
C_eventp event;
C_subeventp subevent;
int qqq[100];
for (int i=0; i<100; i++) qqq[i]=0;
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

event >> setcopy(dt_is);

try
  {
  int weiter=1;
  int geschrieben=0, gelesen=0;
  evout->swap(args->getboolopt("swap"));
  while (weiter && (*evin >> event))
    {
    gelesen++;
    if (event.evnr()!=0)
      {
      if ((event.evnr()%100000)==0) clog<<"event "<<event.evnr()<<endl;
      if ((maxevid>0) && (event.evnr()>maxevid))
        weiter=0;
      else
        {
        if (event >> subevent)
          {
          if (subevent.id()!=dt_is)
            {
            clog << "got SI " << subevent.id() << endl;
            return -1;
            }
          int ldt=subevent[2];
          int tdt=subevent[3];
          int qq=tdt/1000;
          if (qq<100) qqq[qq]++;
          int save=(!usemin || (tdt>=mindt)) && (!usemax || (tdt<=maxdt));
          if (debug)
            {
            clog<<"event "<<event.evnr()<<"; tdt "<<tdt<<"; save "<<save<<endl;
            }
          if (save)
            {
            *evout << event;
            if ((++geschrieben>maxevnum) && (maxevnum>0)) weiter=0;
            }
          }
        else
          {
          clog <<"event "<<event.evnr()<<": kein subevent "<<dt_is<<endl;
          }
        }
      }
    }
  clog << geschrieben << " of " << gelesen << " events stored"
      << "; last event: " << event.levnr() << endl;
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  }
for (int j=0; j<100; j++) clog << j << ": " << qqq[j] << endl;
delete evin;
delete evout;
return 0;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

if (verbose)
  {
  int i;
  if (tapeinput) clog << "input from tape" << endl;
  if (tapeoutput) clog << "output to tape" << endl;
  clog << "name of input file: " << infile << endl;
  clog << "name of output file: " << outfile << endl;
  clog << endl;
  }

do_select();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
