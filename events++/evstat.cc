/******************************************************************************
*                                                                             *
* evstat.cc                                                                   *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 19.08.96                                                           *
* last changed: 20.08.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <events.hxx>
#include "compat.h"

#include "statist.hxx"
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: evstat.cc,v 1.7 2014/07/14 16:18:17 wuestner Exp $")
#define XVERSION

class statistic
  {
  public:
    statistic();
    statistic(const statistic&);
    ~statistic();
  protected:
    statist_int evstat;
    statist_int snumstat;
    statist_int* sevstat;
    int* sevids;
    int sevnum;
    int triggers[16];
  public:
    void add(statistic&);
    void addev(int);
    void addsnum(int);
    void addsev(int, int);
    void addtrigg(int);
    void print(ostream&);
    int markdist(int n) {return evstat.markdist(n);}
  };

C_readargs* args;
int verbose;
int debug;
int ask;
int single;
int istape;
STRING infile;
int numnames;
statistic evstat;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "verbose", false, "verbose");
args->addoption("debug", "debug", false, "debug");
args->addoption("ask", "ask", false, "ask for next file");
args->addoption("single", "single", false, "only one file");
args->addoption("no_tape", "nt", false, "input not from tape");
args->addoption("is_tape", "it", false, "input from tape");
args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");
args->hints(C_readargs::exclusive, "ask", "single", 0);
args->hints(C_readargs::exclusive, "no_tape", "is_tape", 0);

if (args->interpret(1)!=0) return(-1);

verbose=args->getboolopt("verbose");
debug=args->getboolopt("debug");
ask=args->getboolopt("ask");
single=args->getboolopt("single");
istape=args->getboolopt("no_tape");
infile=args->getstringopt("infile");
numnames=args->numnames();
if (numnames>1)
  istape=args->getboolopt("is_tape");
else
  istape=!args->getboolopt("no_tape");
return(0);
}

/*****************************************************************************/

statistic::statistic()
:sevstat(0), sevids(0), sevnum(0)
{
for (int i=0; i<16; i++) triggers[i]=0;
}

/*****************************************************************************/

statistic::statistic(const statistic& st)
{
sevnum=st.sevnum;
if (sevnum)
  {
  sevstat=new statist_int[sevnum];
  sevids=new int[sevnum];
  for (int i=0; i<sevnum; i++) sevids[i]=st.sevids[i];
  }
else
  {
  sevstat=0;
  sevids=0;
  }
for (int i=0; i<16; i++) triggers[i]=st.triggers[i];
}

/*****************************************************************************/

statistic::~statistic()
{
delete[] sevstat;
delete[] sevids;
}

/*****************************************************************************/

void statistic::add(statistic& st)
{
int i;
evstat.add(st.evstat);
snumstat.add(st.snumstat);
for (i=0; i<st.sevnum; i++)
  {
  int j, id=st.sevids[i];
  for (j=0; (j<sevnum) && (sevids[j]!=id); j++);
  if (j<sevnum)
    sevstat[j].add(st.sevstat[i]);
  else
    {
    int k;
    statist_int* shelp=new statist_int[sevnum+1];
    int* ihelp=new int [sevnum+1];
    for (k=0; k<sevnum; k++) shelp[k]=sevstat[k];
    for (k=0; k<sevnum; k++) ihelp[k]=sevids[k];
    delete[] sevstat;
    delete[] sevids;
    sevstat=shelp;
    sevids=ihelp;
    sevstat[sevnum]=st.sevstat[i];
    sevids[sevnum]=id;
    sevnum++;
    }
  }
for (i=0; i<16; i++) triggers[i]+=st.triggers[i];
}

/*****************************************************************************/

void statistic::addev(int i)
{
evstat.add(i);
}

/*****************************************************************************/

void statistic::addsnum(int i)
{
snumstat.add(i);
}

/*****************************************************************************/

void statistic::addsev(int id, int n)
{
int j;
for (j=0; (j<sevnum) && (sevids[j]!=id); j++);
if (j<sevnum)
  {
  sevstat[j].add(n);
  }
else
  {
  int k;
  statist_int* shelp=new statist_int[sevnum+1];
  int* ihelp=new int [sevnum+1];
  for (k=0; k<sevnum; k++) shelp[k]=sevstat[k];
  for (k=0; k<sevnum; k++) ihelp[k]=sevids[k];
  delete[] sevstat;
  delete[] sevids;
  sevstat=shelp;
  sevids=ihelp;
  sevstat[sevnum].add(n);
  sevids[sevnum]=id;
  sevnum++;
  }
}

/*****************************************************************************/

void statistic::addtrigg(int t)
{
if ((t>=0) && (t<16))
  triggers[t]++;
else
  clog << "trigger " << t << " nicht moeglich" << endl;
}

/*****************************************************************************/

void statistic::print(ostream& st)
{
int i;
st << "events:" << endl << "  ";
evstat.print(st);
st << "subevents:" << endl << "  ";
snumstat.print(st);
for (i=0; i<sevnum; i++)
  {
  st << sevids[i] << ":  ";
  sevstat[i].print(st);
  }
st << "triggers:"<<endl;
for (i=0; i<16; i++)
  {
  if (triggers[i])
    {
    clog << i <<": " << triggers[i] << " ("
        <<(float)triggers[i]/(float)evstat.getnum()*100.0<<"%)  ";
    }
  }
clog << endl;
}

/*****************************************************************************/

void scan_file(C_evinput* evin, int& fnum, const char* name)
{
C_eventp event;
C_subeventp subevent;

fnum++;
statistic fevstat(evstat);
fevstat.markdist(1024*1024);
try
  {
  while ((*evin) >> event)
    {
    if (event.evnr()!=0)
      {
      fevstat.addtrigg(event.trignr());
      fevstat.addev(event.size());
      int n=0;
      while (event >> subevent)
        {
        n++;
        fevstat.addsev(subevent.id(), subevent.size());
        }
      fevstat.addsnum(n);
      }
    }
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  }

if (name)
  cout << endl << "### file " << name << " ###" << endl;
else
  cout << endl << "### file " << fnum << " ###" << endl;
fevstat.print(cout);
if (!single) evstat.add(fevstat);
}

/*****************************************************************************/

void scan_tape()
{
C_evinput* evin;

try
  {
  if (debug) clog << "try to open" << endl;
  if (istape)
    {
    if (debug) clog << "open tape" << endl;
    evin=new C_evtinput(infile, 65536);
    }
  else
    {
    if (debug) clog << "open buffered" << endl;
    evin=new C_evfinput(infile, 65536);
    }
  if (debug) clog << "open ok" << endl;
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return;
  }

int fnum=0;
if (single)
  {
  scan_file(evin, fnum, 0);
  }
else
  {
  while (!evin->fatal())
    {
    scan_file(evin, fnum, 0);
    cout << "--- gesamt ---" << endl;
    evstat.print(cout);
    evin->reset();
    if (ask)
      {
      int dummy;
      clog << "press enter for next file! " << flush;
      cin >> dummy;
      }
    }
  }

delete evin;
}

/*****************************************************************************/

void scan_files()
{
C_evinput* evin;
int fnum=0;
for (int i=0; i<numnames; i++)
  {
  try
    {
    const char* name=args->getnames(i);
    
    if (debug) clog << "try to open " << name << endl;
    if (istape)
      {
      if (debug) clog << "open tape" << endl;
      evin=new C_evtinput(name, 65536);
      }
    else
      {
      if (debug) clog << "open buffered" << endl;
      evin=new C_evfinput(name, 65536);
      }
    if (debug) clog << "open ok" << endl;
    scan_file(evin, fnum, name);
    cout << "--- gesamt ---" << endl;
    evstat.print(cout);
    evin->reset();
    if (ask)
      {
      int dummy;
      clog << "press enter for next file! " << flush;
      cin >> dummy;
      }
    
    }
  catch (C_error* err)
    {
    cerr << (*err) << endl;
    delete err;
    return;
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

if (numnames>1)
  scan_files();
else
  scan_tape();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
