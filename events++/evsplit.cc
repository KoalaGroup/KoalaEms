/******************************************************************************
*                                                                             *
* evsplit.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 04.11.96                                                           *
* last changed: 04.11.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <events.hxx>
#include <config.h>
#include <compat.h>
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: evsplit.cc,v 1.6 2005/02/11 15:45:06 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile;
STRING outfile;

C_evinput* evin;
C_evoutput* evout;
int evmax;

/*****************************************************************************/

int readargs()
{
args->addoption("maxevnum", "evmax", -1, "maximum number of events to be"
   " processed", "events");
args->addoption("infile", 0, "", "input file", "input");
args->addoption("outfile", 1, "", "stub foroutput file", "output");
args->hints(C_readargs::required, "infile", "outfile", 0);

if (args->interpret(1)!=0) return -1;

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
evmax=args->getintopt("maxevnum");
return 0;
}

/*****************************************************************************/

int do_open()
{
try
  {
  evin=new C_evfinput(infile, 65536);
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return(-1);
  }
return 0;
}

/*****************************************************************************/

int do_reed(C_eventp& event)
{
try
  {
  do
    {
    (*evin) >> event;
    }
  while (event.evnr()==0);
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  return -1;
  }
return 0;
}

/*****************************************************************************/

void do_proceed()
{
C_eventp event;
int filecount=0;
int weiter=1;
int evid=-1, oldevid=0;

if (do_open()<0) return;
try
  {
  SSTREAM st;
  st << outfile << '_' << filecount << ends;
  STRING ss=st.str();
  evout=new C_evfoutput(ss.c_str(), 65536);
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  return;
  }
do
  {
  if (do_reed(event)<0)
    {
    delete evout;
    return;
    }
  if ((evmax>=0) && (event.evnr()>evmax))
    {
    delete evout;
    return;
    }
  evid=event.evnr();
  if (evid!=++oldevid)
    {
    cout << "Sprung: " << oldevid-1 << " <==> " << evid << endl;
    oldevid=evid;
    delete evout;
    filecount++;
    try
      {
      SSTREAM st;
      st << outfile << '_' << filecount << ends;
      STRING ss=st.str();
      evout=new C_evfoutput(ss.c_str(), 65536);
      }
    catch (C_error* error)
      {
      clog << (*error) << endl;
      delete error;
      return;
      }
    }
  if (evid%1000==0) cout << "event " << evid << endl;
  (*evout) << flush << event << flush;
  }
while (weiter);
cout << "last id: " << evid << endl;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

do_proceed();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
