/******************************************************************************
*                                                                             *
* dtscan.cc                                                                   *
*                                                                             *
* created: 13.01.97                                                           *
* last changed: 14.01.97                                                      *
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
#include <events.hxx>
#include <compat.h>
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: deadtrig.cc,v 1.6 2005/02/11 15:44:58 wuestner Exp $")
#define XVERSION

C_readargs* args;
int debug;
int tape;
//int sevid;
//int offset;
STRING infile;

class hist
  {
  public:
    hist(const char* name, int binsize=0);
    ~hist();
  protected:
    STRING name_;
    int bsize;
    int numbins, usedbins, lastbin;
    struct pair
      {
      long bin;
      long val;
      int valid;
      };
    pair* list;
    double sum;
    double qsum;
  public:
    const STRING& name() const {return name_;}
    void incr(long idx);
    void sort(void);
    long last(void) const {return lastbin;}
    int binsize(void) const {return bsize;}
    long operator[](long);
    long restsum(long start=0) const;
    void save(ostream&) const;
    void save(ostream&, long max) const;
    double getsum(void) const {return sum;}
    double getqsum(void) const {return qsum;}
  };

//hist deadtime(8);
hist timediff("time", 10);
hist triggered("trig");

/*****************************************************************************/
/*****************************************************************************/

hist::hist(const char* name, int binsize)
:name_(name),
  bsize(binsize), numbins(100), usedbins(0), lastbin(-1), sum(0), qsum(0)
{
list=new pair[numbins];
}

/*****************************************************************************/

hist::~hist()
{
delete[] list;
}

/*****************************************************************************/

void hist::incr(long idx)
{
//if (debug) clog << "hist("<<name_<<")::incr("<<idx<<")"<<endl;
sum+=idx;
qsum+=idx*idx;
if (bsize) idx/=bsize;
int i=0;
while ((i<usedbins) && (list[i].bin!=idx)) i++;
if (i<usedbins)
  list[i].val++;
else
  {
  if (usedbins<numbins)
    {
    list[usedbins].bin=idx;
    list[usedbins].val=1;
    usedbins++;
    }
  else
    {
    pair* help=new pair[numbins+100];
    for (i=0; i<numbins; i++) help[i]=list[i];
    delete[] list;
    list=help;
    numbins+=100;
    list[usedbins].bin=idx;
    list[usedbins].val=1;
    usedbins++;
    }
  if (lastbin<idx)
    {
    lastbin=idx;
    //if (debug) clog << "new lastbin("<<name_<<"): "<<lastbin<<endl;
    }
  }
}

/*****************************************************************************/

void hist::sort()
{
int i, n, newidx=0;

for (i=0; i<usedbins; i++) list[i].valid=1;
pair* help=new pair[numbins];
clog << "usedbins: "<<usedbins<<endl;
for (n=0; n<usedbins; n++)
  {
  int j=0;
  while (!list[j].valid) j++;
  long lastbin=list[j].bin;
  int lastidx=j;
  for (j++; j<usedbins; j++)
    if ((list[j].valid) && (list[j].bin<lastbin))
      {
      lastbin=list[j].bin;
      lastidx=j;
      }
  help[newidx++]=list[lastidx];
  list[lastidx].valid=0;
  }
delete[] list;
list=help;
}

/*****************************************************************************/

void hist::save(ostream& os) const
{
for (int i=0; i<usedbins; i++)
  {
  os << list[i].bin << "  " << list[i].val << endl;
  }
}

/*****************************************************************************/

void hist::save(ostream& os, long max) const
{
int idx=0;
long val;
for (long m=0; m<=max; m++)
  {
  while ((idx<usedbins) && (list[idx].bin<m)) idx++;
  if (idx==usedbins) return;
  if (list[idx].bin==m)
    val=list[idx].val;
  else
    val=0;
  os << m << "  " << val << endl;
  }
}

/*****************************************************************************/

long hist::operator[](long idx)
{
if (idx>lastbin) return 0;
int i=0;
while ((i<usedbins) && (list[i].bin!=idx)) i++;
if (i<usedbins)
  return list[i].val;
else
  return 0;
}

/*****************************************************************************/
/*****************************************************************************/

int readargs()
{
// arg fuer daten:
// -s xx -o xx -f-o xx ... -s ...
// -s: subevent
// -o: offset
// -t: type (a: absolute, d: difference)
// -f: outputfile
// -m: maxbin
// -b: binsize

args->addoption("debug", "debug", false, "debug");
args->addoption("tape", "tape", false, "input from tape");
args->addoption("maxrec", "maxrec", 65536, "maxrec", "max. recordsize");
//args->addoption("lastbin", "lastbin", 100, "last bin");
args->addoption("infile", 0, "", "input file", "input");
//args->hints(C_readargs::required, "subevent", 0);

if (args->interpret(1)!=0) return(-1);

debug=args->getboolopt("debug");

tape=(args->getboolopt("tape"));
infile=args->getstringopt("infile");
//sevid=args->getintopt("subevent");
//offset=args->getintopt("offset");
//lastbin=args->getintopt("lastbin");
return(0);
}

/*****************************************************************************/

void scan_file(C_evinput* evin, int& fnum)
{
C_eventp event;
C_subeventp subevent;
int numevents=0;
int zeit, trigger, diff;
int last_time=-1;
int last_trigger=-1;

try
  {
  while ((*evin) >> event)
    {
    if (event.evnr()!=0)
      {
      numevents++;
      //if (debug) clog << "event " << event.evnr() << endl;
//       if (event.getsubevent(subevent, 10)!=0)
//         {
//         clog << "event " << event.evnr() << ": no subevent " << 10 << endl;
//         return;
//         }
//       if (subevent.dsize()!=2)
//         {
//         clog <<"event " << event.evnr() << "; sev " << 10 << ": size="
//             << subevent.dsize() << endl;
//         }
//       deadtime.incr(subevent[3]);
      
      if (event.getsubevent(subevent, 12)!=0)
        {
        clog << "event " << event.evnr() << ": no subevent " << 12 << endl;
        return;
        }
//       if (subevent.dsize()<3)
//         {
//         clog << "event " << event.evnr() << "; sev " << 12 << ": size="
//             << subevent.dsize() << endl;
//         clog << hex;
//         for (int i=0; i<subevent.size(); i++) clog << subevent[i] << endl;
//         clog << dec;
//         }
      if (debug)
        {
        for (int i=0; i<5; i++) clog << setw(8) << (subevent[i]&0xffffff) <<" ";
        clog << endl;
        }
      if ((subevent[1]!=4)
          || ((subevent[2]&0xffffff)==0)
          || ((subevent[3]&0xffffff)==0)
          || ((subevent[4]&0xffffff)!=0))
        {
        last_time=-1;
        last_trigger=-1;
        }
      else
        {
        int trigok=0;
        if (last_trigger>=0)
          {
          trigger=subevent[3]&0xffffff;
          diff=trigger-last_trigger;
          if (diff>=0) triggered.incr(diff);
          trigok=diff==1;
          last_trigger=trigger;
          }
        else
          {
          last_trigger=subevent[3]&0xffffff;
          clog << "last_trigger="<<last_trigger<<endl;
          }
        if (trigok && (last_time>=0))
          {
          zeit=subevent[2]&0xffffff;
          diff=zeit-last_time;
          if (diff>=0) timediff.incr(diff);
          last_time=zeit;
          }
        else
          last_time=subevent[2]&0xffffff;
       }

      if (numevents%10000==0)
        {
        clog << numevents << "; last_trigger="<<last_trigger<< endl;
        }
      }
    }
  clog << numevents << " events processed" << endl;
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
  if (tape)
    evin=new C_evtinput(infile, 65536);
  else
    evin=new C_evfinput(infile, 65536);
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return;
  }

int fnum=0;
if (!tape)
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

int output(hist& histo, const char* name)
{
ofstream* ofile=new ofstream(name);
if (!ofile->good())
  {
  clog << "can't open " << name << " for writing: ???" << endl;
  return -1;
  }
histo.sort();
histo.save(*ofile,1000);
// long lastbin=histo.last();
// for (long i=0; i<=lastbin; i++)
//   {
//   if (histo[i]) *ofile << i << "  " << histo[i] << endl;
//   }
delete ofile;
return 0;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
int i;

args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);


scan_tape();

// output(deadtime, "deadtime");
output(timediff, "timediff");
output(triggered, "triggered");

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
