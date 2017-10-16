/******************************************************************************
*                                                                             *
* dtscan.cc                                                                   *
*                                                                             *
* created: 06.12.96                                                           *
* last changed: 06.12.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include "events.hxx"
#include "compat.h"
#include <versions.hxx>

VERSION("May 23 2001", __FILE__, __DATE__, __TIME__,
"$ZEL: dtscan.cc,v 1.7 2005/02/11 15:44:59 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int sevid;
STRING infile;
//STRING outfile;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "verbose", false, "verbose");
args->addoption("subevent", "subevent", 0, "subevent id", "subevent");
args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
args->addoption("infile", 0, "", "input file", "input");
args->addoption("ilit", "iliterally", false,
    "use name of input file literally", "input file literally");
//args->addoption("outfile", 1, "deadtime.hist", "output file", "output");
args->hints(C_readargs::required, "subevent", "infile", 0);

if (args->interpret(1)!=0) return(-1);

verbose=args->getboolopt("verbose");

infile=args->getstringopt("infile");
//outfile=args->getstringopt("outfile");
sevid=args->getintopt("subevent");
return(0);
}

/*****************************************************************************/

// int openoutfile()
// {
// #ifdef USE_STRING_H
// ofile=new ofstream(outfile);
// #else
// ofile=new ofstream(outfile.c_str());
// #endif
// if (!ofile->good())
//   {
//   cerr << "can't open " << outfile << " for writing: ???" << endl;
//   return -1;
//   }
// return 0;
// }

/*****************************************************************************/
// format of subevent GetPCITrigData:
// 
//      struct trigprofdata
//        {
//        int flagnum;
//        unsigned int flags[4];
//        int timenum;
//        unsigned int times[7];
//        };
//  [0]: subevent_id
//  [1]: size
//      struct trigstatus {
//  [2]:     unsigned int mbx;
//  [3]:     unsigned int state;
//  [4]:     unsigned int evc;
//  [5]:     unsigned int tmc;
//  [6]:     unsigned int tdt;
//  [7]:     unsigned int ldt;
//  [8]:     unsigned int reje;
// [9 10 11]:     unsigned int tgap[3];
//      #ifdef PROFILE
//        struct trigprofdata data;
//      #endif
//      };
// 
//      struct trigstatus
//      #ifdef PROFILE
//      struct trigprofdata_u
//      #else
// [12]: 0
// [13]: 0
//      #endif
// [14]: 3
// [15]: 0
//      #ifdef SCHED_TRIGGER
// [16]: suspensions
// [17]: after_suspension
//      #endif

void scan_file(C_evinput* evin)
{
C_eventp event;
C_subeventp subevent;
int numevents=0;

try
  {
  event >> setcopy(sevid);
  while ((*evin) >> event)
    {
    if (event.evnr()!=0)
      {
      numevents++;
      event >> subevent;
      if (!subevent.isvalid())
        {
        cerr << "event " << event.evnr() << ": no subevent " << sevid << endl;
        //throw new C_program_error(os);
        }
      
      if (subevent.dsize()!=16)
        {
        OSSTREAM os;
        os << "event " << event.evnr()
            << ": subevent contains " << subevent.dsize() << " words" << endl;
        throw new C_program_error(os);
        }
      
      int evc, tdt, ldt, reje;
      evc=subevent[4];
      tdt=subevent[6];
      ldt=subevent[7];
      reje=subevent[8];
      cout<<"evc="<<evc<<" tdt="<<tdt<<" ldt="<<ldt<<" reje="<<reje<<endl;
//       for (int i=0; i<16; i++)
//         {
//         cout<<"["<<i<<"]:"<<subevent[i];
//         }
//       cout<<endl;
      }
    }
  cerr << numevents << " processed" << endl;
  if (evin->is_tape())
    {
    C_evtinput* in=(C_evtinput*)evin;
    if (!in->fatal() && in->filemark()) cerr << "filemark" << endl;
    }
  }
catch (C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  }
}

/*****************************************************************************/

void scan_events(C_iopath& reader)
{
C_evinput* evin;

try
  {
  if (reader.typ()!=C_iopath::iotype_tape)
    evin=new C_evfinput(reader.path(), 65536);
  else
    evin=new C_evtinput(reader.path(), 65536);
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return;
  }

scan_file(evin);
delete evin;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

C_iopath reader(infile, C_iopath::iodir_input, args->getboolopt("ilit"));

scan_events(reader);

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
