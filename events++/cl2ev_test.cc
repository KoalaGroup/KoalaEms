/******************************************************************************
*                                                                             *
* cl2ev_test.cc                                                               *
*                                                                             *
* created: 30.03.98                                                           *
* last changed: 30.03.98                                                      *
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
#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include <strstream.h>
#include <readargs.hxx>
#include <errno.h>
#include <sys/ioctl.h>
#include <strstream.h>
#include "events.hxx"
#include "compat.h"

C_readargs* args;
int verbose;
int debug;
int tapeinput;
STRING infile;
int recsize;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("tape_in", "ti", false, "input is tape");
args->addoption("buffered_in", "bi", false, "input is buffered",
    "buffered_input");
args->addoption("buffered_out", "bo", false, "output is buffered",
    "buffered_output");
args->addoption("maxexpect", "maxexp", 0, "maximum expected eventsize",
    "maxexp");
args->addoption("recsize", "recsize", 65536/4, "record size", "recsize");
args->addoption("infile", 0, "-", "input file (- - for stdin)", "input");
args->hints(C_readargs::implicit, "debug", "verbose", 0);

if (args->interpret(1)!=0) return -1;

debug=args->getboolopt("debug");
verbose=args->getboolopt("verbose");
tapeinput=args->getboolopt("tape_in");
infile=args->getstringopt("infile");

recsize=args->getintopt("recsize");
return(0);
}

/*****************************************************************************/

int do_select()
{
C_evinput* evin;
C_eventp event;
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
  int weiter=1;
  int gelesen=0;

  while (weiter && (*evin >> event))
    {
    gelesen++;
    if (event.evnr()!=0)
      {
      int is[5]={0,0,0,0,0};
      if ((event.evnr()%1000)==0) cerr<<"event "<<event.evnr()<<endl;
      while (event >> subevent)
        {
        if ((subevent.id()<1) || (subevent.id()>4))
          cerr<<"found sev "<<subevent.id()<<" in event "<<event.evnr()<<endl;
        else
          is[subevent.id()]=1;
        /*
         * subevent[0]: id
         * subevent[1]: size
         * subevent[2]: id
         * subevent[3]: size of data
         * ...
         * subevent[x-4]: 5
         * subevent[x-3]: 4
         * subevent[x-2]: 3
         * subevent[x-1]: 2
         * subevent[  x]: 1 
         */
        if (subevent[0]!=subevent[2])
          {
          cerr<<"event "<<event.evnr()<<"; sev "<<subevent.id()
              <<": subevent[2]="<<subevent[2]<<endl;
          }
        if (subevent[1]!=subevent[3]+2)
          {
          cerr<<"event "<<event.evnr()<<"; sev "<<subevent.id()
              <<": size="<<subevent[1]<<"; subevent[3]="<<subevent[3]<<endl;
          }
        for (int i=0, j=subevent[3]; i<subevent[3]; i++, j--)  
          {
          int v=event.evnr()<<16|((subevent.id()<<12)&0xf000)|(j&0xfff);
          if (subevent[i+4]!=v)
            {
            cerr<<"event "<<event.evnr()<<"; sev "<<subevent.id()
              <<": idx="<<i<<"; subevent[idx+4]="
              <<hex<<(unsigned int)(subevent[i+4])<<dec<<endl;
            }
          }
        }
      for (int i=1; i<5; i++) if (is[i]==0)
        {
        cerr<<"event "<<event.evnr()<<": sev"<<i<<" fehlt"<<endl;
        }
      }
    else
      {
      cerr<<"eventid=0"<<endl;
      }
    }
  cerr<<gelesen<<" events gelesen; last event: "<<event.levnr()<<endl;
  }
catch (C_error* error)
  {
  cerr<<(*error)<<endl;
  delete error;
  }
delete evin;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

if (verbose)
  {
  int i;
  if (tapeinput) cerr << "input from tape" << endl;
  cerr << "name of input file: " << infile << endl;
  cerr << endl;
  }

do_select();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
