/******************************************************************************
*                                                                             *
* timescan.cc                                                                 *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 13.11.96                                                           *
* last changed: 13.11.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#if defined  (__STD_STRICT_ANSI) || defined (__STRICT_ANSI__)
#include <iostream>
#include <fstream>
#include <strstream>
#include <iomanip>
#include <string>
#define STRING string
#else
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include <iomanip.h>
#include <String.h>
#define USE_STRING_H
#define STRING String
#endif

#include <readargs.hxx>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "events.hxx"
#include "compat.h"
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: timescan.cc,v 1.5 2005/02/11 15:45:15 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
STRING data_stub;
STRING commfilename;
int single;
int buffered;
int sevid;
STRING infile;
ofstream* cfile;
ofstream* dfile;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "verbose", false, "verbose");
args->addoption("data_stub", "data_stub", "timedata", "stub for data file", "stub");
args->addoption("commandfile", "commandfile", "ev.kumac", "command file", "comm");
args->addoption("subevent", "subevent", 0, "subevent id", "subevent");
args->addoption("single", "single", false, "only one file");
args->addoption("no_tape", "nt", false, "input not from tape");
args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");
args->hints(C_readargs::required, "subevent", 0);

if (args->interpret(1)!=0) return(-1);

verbose=args->getboolopt("verbose");
commfilename=args->getstringopt("commandfile");
data_stub=args->getstringopt("data_stub");

single=args->getboolopt("single");
buffered=(args->getboolopt("no_tape"));
infile=args->getstringopt("infile");
sevid=args->getintopt("subevent");
return(0);
}

/*****************************************************************************/

int opencommfile()
{
#ifdef USE_STRING_H
cfile=new ofstream(commfilename);
#else
cfile=new ofstream(commfilename.c_str());
#endif
if (!cfile->good())
  {
  clog << "can't open " << commfilename << " for writing: ???" << endl;
  return -1;
  }
dfile=0;
return 0;
}

/*****************************************************************************/

void scan_file(C_evinput* evin, int& fnum)
{
C_eventp event;
C_subeventp subevent;
int numevents=0;
int wrote_line=0;
static long lastmsec=0, lastpulser=0, spill_start=0, bin_start=0, sum_time=0;
static int spill_nr=0, bin_cont=0;
static char ts1[1024], ts2[1024], *dname=0;
static float max;
int binnum;

try
  {
  event >> setcopy(sevid);
  while ((*evin) >> event)
    {
    if (!wrote_line)
      {
      if (!single) cerr << "File " << ++fnum << endl;
      wrote_line=1;
      }
    if (event.evnr()!=0)
      {
      numevents++;
      event >> subevent;
      if (!subevent.isvalid())
        {
        ostrstream os;
        os << "event " << event.evnr() << ": no subevent " << sevid << endl;
        throw new C_program_error(os);
        }
      if (subevent.dsize()!=4)
        {
        ostrstream os;
        os << "event " << event.evnr()
            << ": subevent contains " << subevent.dsize() << " words" << endl;
        throw new C_program_error(os);
        }

      int tage, sekunden, hundertstel;
      long msec, pulser;
      struct timeval tv;

      pulser=(unsigned int)subevent[2];
      tage=subevent[3];
      sekunden=subevent[4];
      hundertstel=subevent[5];
      tv.tv_sec=(tage-2440587)*86400+sekunden-3600;
      tv.tv_usec=(hundertstel&0xffff)*10000;
      msec=(long)tv.tv_sec*1000+(hundertstel&0xffff)*10;

      int newspill=(msec-lastmsec)>3000;
      int newbin=(msec-bin_start)>5000;
      if (newspill || newbin)
        {
        if (bin_start>0)
          {
          //float rate=(float)bin_cont/(lastmsec-bin_start)*1000;
          float rate=(float)bin_cont/5;
          if (rate>max) max=rate;
          (*dfile) << (float)(bin_start-spill_start)/1000 << ' ' << rate << endl;
          binnum++;
          }
        bin_cont=1;
        //bin_start=lastmsec;
        bin_start+=5000;
        }
      else
        bin_cont++;
      if (newspill)
        {
        struct tm *tm;

        if (spill_start>0)
          {
          int ymax;
          time_t sec=lastmsec/1000;
          tm=localtime(&sec);
          strftime(ts2, 1024, "%H.%M:%S", tm);
          long spill_time=lastmsec-spill_start;
          sum_time+=spill_time;
          (*dfile) << "0 0" << endl;
          (*dfile) << "sum_time " << (float)spill_time/1000 << " s" << endl;
          if (max<20)
            ymax=20;
          else if (max<50)
            ymax=50;
          else if (max<100)
            ymax=100;
          else if (max<200)
            ymax=200;
          else if (max<500)
            ymax=500;
          else if (max<1000)
            ymax=1000;
          //cout << "%0 " << spill_time/1000 << ' ' << sum_time/1000 << endl;
          (*cfile) << "exec evrate#plot " << dname << ' ' << binnum
              << ' ' << 400 << ' ' << ymax << " '"
              << ts1 << " - " << ts2 << "; (" 
              << (float)spill_time/1000 <<  " s; "
              << (float)(msec-lastmsec)/1000<< " s pause)\' "
              << spill_nr << endl;
          }
        spill_start=msec;
        bin_start=msec;
        tm=localtime(&tv.tv_sec);
        strftime(ts1, 1024, "%m/%d %H.%M:%S", tm);
        //cout << "%1 " << spill_nr << endl;
        //cout << "%2 " << ts << endl;
        if (dfile) delete dfile;
        spill_nr++;
        ostrstream os;
        os << data_stub << spill_nr << ends;
        if (dname) delete[] dname;
        dname=os.str();
        dfile=new ofstream(dname);
        binnum=0;
        max=0;
        }
      lastmsec=msec;
      lastpulser=pulser;
      }
    }
  clog << numevents << " processed" << endl;
  if (evin->is_tape())
    {
    C_evtinput* in=(C_evtinput*)evin;
    if (!in->fatal() && in->filemark()) clog << "filemark" << endl;
    }
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  }
}

/*****************************************************************************/

void scan_tape()
{
C_evinput* evin;

try
  {
  if (buffered)
    evin=new C_evfinput(infile, 65536);
  else
    evin=new C_evtinput(infile, 65536);
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return;
  }

int fnum=0;
if (single)
  scan_file(evin, fnum);
else
  {
  while (!evin->fatal())
    {
    scan_file(evin, fnum);
    evin->reset();
   }
  }
delete evin;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);
if (opencommfile()!=0) return(0);

scan_tape();

if (dfile) delete dfile;
delete cfile;
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
