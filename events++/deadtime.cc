/******************************************************************************
*                                                                             *
* deadtime.cc                                                                 *
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
#include <errno.h>
#include <readargs.hxx>
#include <events.hxx>
#include <compat.h>
#include "versions.hxx"

VERSION("Jul 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: deadtime.cc,v 1.6 2005/02/11 15:44:58 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile1;
STRING infile2;
STRING outfile;
int evmax;

C_evinput* evin1;
C_evinput* evin2;

class evdata
  {
  public:
    evdata(int);
    ~evdata();
    void clear();
    void sevresize(int);
    void addsevsize(int, int);
    void print(ostream&);
  public:
    int evnum;
    int ldt;
    int tdt;
    int sevsizelen;
    int maxsev;
    int* sevsize;
    int size_1881;
    int num_1881;
    int gsum_1881;
    int size_1877;
    int num_1877;
    int size_1875;
    int num_1875;
    int gsum_1875;
    int size_1879;
    int num_1879;
    int fbslots[26];
  };

/*****************************************************************************/

int readargs()
{
args->addoption("maxevnum", "evmax", -1, "maximum number of events to be"
   " processed", "events");
args->addoption("binsize", "bsize", 10, "bin size", "binsize");
args->addoption("binnum", "bnum", 10000, "number of bins", "binnum");
args->addoption("dobin", "bin", false, "create statistic");
args->addoption("infile_dt", 0, "", "input file for deadtime", "input1");
args->addoption("infile_data", 1, "", "input file for data", "input2");
args->addoption("outfile", 2, "", "output file", "output");
args->hints(C_readargs::required, "infile_dt", "infile_data", "outfile", 0);
args->hints(C_readargs::implicit, "binsize", "dobin", 0);
args->hints(C_readargs::implicit, "binnum", "dobin", 0);
if (args->interpret(1)!=0) return -1;

infile1=args->getstringopt("infile_dt");
infile2=args->getstringopt("infile_data");
outfile=args->getstringopt("outfile");
evmax=args->getintopt("maxevnum");
return 0;
}

/*****************************************************************************/

evdata::evdata(int n)
:sevsizelen(n), maxsev(-1)
{
sevsize=new int[sevsizelen];
for (int i=0; i<sevsizelen; i++) sevsize[i]=-1;
}

/*****************************************************************************/

evdata::~evdata()
{
if (sevsize) delete[] sevsize;
}

/*****************************************************************************/

void evdata::sevresize(int n)
{
int* help=new int[n];
for (int i=0; (i<n)&&(i<sevsizelen); i++) help[i]=sevsize[i];
delete[] sevsize;
sevsize=help;
}

/*****************************************************************************/

void evdata::clear()
{
maxsev=-1;
for (int i=0; i<sevsizelen; i++) sevsize[i]=-1;
size_1881=0;
num_1881=0;
gsum_1881=0;
size_1877=0;
num_1877=0;
size_1875=0;
num_1875=0;
gsum_1875=0;
size_1879=0;
num_1879=0;
for (int j=0; j<26; j++) fbslots[j]=0;
}

/*****************************************************************************/

void evdata::addsevsize(int sevid, int size)
{
if (sevsizelen<=sevid) sevresize(sevid+1);
sevsize[sevid]=size;
if (sevid>maxsev) maxsev=sevid;
}

/*****************************************************************************/

void evdata::print(ostream& os)
{
os << evnum << ' ' << tdt;       //  2
// for (int i=0; i<=maxsev; i++)
//   {
//   os << ' ' << sevsize[i];
//   }
os << ' ' << sevsize[1];         //  3
os << ' ' << sevsize[2];         //  4
os << ' ' << size_1881;          //  5
os << ' ' << size_1877;          //  6
os << ' ' << size_1875;          //  7
os << ' ' << size_1879;          //  8
os << ' ' << num_1881;           //  9
os << ' ' << gsum_1881;          // 10
os << ' ' << num_1877;           // 11
os << ' ' << num_1875;           // 12
os << ' ' << gsum_1875;          // 13
os << ' ' << num_1879;           // 14
os << ' ' << size_1875+size_1881;// 15
os << ' ' << num_1875+num_1881;  // 16
os << endl;
}

/*****************************************************************************/

int do_open()
{
try
  {
  evin1=new C_evfinput(infile1, 65536);
  evin2=new C_evfinput(infile2, 65536);
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

int do_reed(C_eventp& event1, C_eventp& event2)
{
try
  {
  do
    {
    (*evin1) >> event1;
    }
  while (event1.evnr()==0);
  do
    {
    (*evin2) >> event2;
    }
  while (event2.evnr()==0);
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
C_eventp event1;
C_eventp event2;
C_subeventp subevent;
evdata evdat(10);
int weiter=1, first=1;
int evid, oldevid=0;
int dobin=0, binsize, binnum;
int* statist;
float* statist_sevsize1;
float* statist_sevsize2;
float* statist_size_1881;
float* statist_size_1877;
float* statist_size_1875;
float* statist_size_1879;
float* statist_num_1881;
float* statist_gsum_1881;
float* statist_num_1877;
float* statist_num_1875;
float* statist_gsum_1875;
float* statist_num_1879;
float* statist_size_sum;
float* statist_num_sum;
int statist_fbslots[26];
int goodevents=0;
ofstream ofs(outfile.c_str());
if (!ofs)
  {
  cout << "bad ofstream" << endl;
  return;
  }

if (do_open()<0) return;

if (args->getboolopt("dobin"))
  {
  int i;
  dobin=1;
  binsize=args->getintopt("binsize");
  binnum=args->getintopt("binnum");
  cout << "binsize=" << binsize << "; binnum=" << binnum << endl;

  statist=new int[binnum+1];
  if (statist==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist[i]=0;

  statist_sevsize1=new float[binnum+1];
  if (statist_sevsize1==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_sevsize1[i]=0;

  statist_sevsize2=new float[binnum+1];
  if (statist_sevsize2==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_sevsize2[i]=0;

  statist_size_1881=new float[binnum+1];
  if (statist_size_1881==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_size_1881[i]=0;

  statist_size_1877=new float[binnum+1];
  if (statist_size_1877==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_size_1877[i]=0;

  statist_size_1875=new float[binnum+1];
  if (statist_size_1875==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_size_1875[i]=0;

  statist_size_1879=new float[binnum+1];
  if (statist_size_1879==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_size_1879[i]=0;

  statist_num_1881=new float[binnum+1];
  if (statist_num_1881==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_num_1881[i]=0;

  statist_gsum_1881=new float[binnum+1];
  if (statist_gsum_1881==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_gsum_1881[i]=0;

  statist_num_1877=new float[binnum+1];
  if (statist_num_1877==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_num_1877[i]=0;

  statist_num_1875=new float[binnum+1];
  if (statist_num_1875==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_num_1875[i]=0;

  statist_gsum_1875=new float[binnum+1];
  if (statist_gsum_1875==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_gsum_1875[i]=0;

  statist_num_1879=new float[binnum+1];
  if (statist_num_1879==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_num_1879[i]=0;

  statist_size_sum=new float[binnum+1];
  if (statist_size_sum==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_size_sum[i]=0;

  statist_num_sum=new float[binnum+1];
  if (statist_num_sum==0) {cout<<"alloc: "<<strerror(errno)<<endl;return;}
  for (i=0; i<=binnum; i++) statist_num_sum[i]=0;

  for (i=0; i<26; i++) statist_fbslots[i]=0;
  }

while (weiter && (do_reed(event1, event2)==0))
  {
  if (first) {(*evin1) >> event1; first=0;}
  if ((evid=event2.evnr())!=(event1.evnr()-1))
    {
    cout << "Mismatch: " << event1.evnr() << " <==> " << event2.evnr() << endl;
    weiter=0;
    }
  //cout << "got " <<evid<< endl;
  if (evid!=++oldevid)
    {
    cout << "Sprung: " << oldevid-1 << " <==> " << evid << endl;
    oldevid=evid;
    weiter=0;
    }
  if (evid%10000==0) cout << "event " << evid << endl;
  if ((evmax>0)&&(evid>evmax)) weiter=0;
  if (weiter)
    {
    if (event1.sevnr()!=1)
      {
      cout << "event1.sevnr: " << event1.sevnr() << endl;
      weiter=0;
      }
    if (event2.sevnr()!=4)
      {
      cout << "event2.sevnr: " << event2.sevnr() << endl;
      weiter=0;
      }
    }
  if (weiter)
    {
    int good=1;
    evdat.evnum=evid;
    event1 >> subevent;
    if (subevent.dsize()!=2)
      {
      cout << "ev1_sev.size()=" << subevent.size() << endl;
      }
    evdat.ldt=subevent[2];
    evdat.tdt=subevent[3];
    while (event2 >> subevent)
      {
      //if (evid<10) cout << "ev: " << evid << "; sev: " << subevent.id() << endl;
      evdat.addsevsize(subevent.id(), subevent.dsize());
      if (subevent.id()==1) // LeCroy
        {
        int n, idx, gsum=0;
        idx=2; n=subevent[idx]; idx++;
        evdat.size_1881=n;
          {
          //cout << "event "<<evid<<"; typ 1881"<< endl;
          //for (int x=idx; x<idx+n; x++) cout << hex << subevent[x] <<dec<<endl;
          // header: adresse<31:27> wordcount<6:0>
          //  datenworte
          // header
          //  ...
          int i=idx, g, h, m=0;
          while (i-idx<n)
            {
            h=subevent[i];
            m++;
            g=(h>>27)&0x1f;
            evdat.fbslots[g]=1;
            //cout << "  x="<<i-idx<<"; g=" << g << "; "
            if ((g<3) || (g>8))
              {
              cout<<"1881: g="<<g<<endl;
              }
            gsum+=g;
            i+=h&0x7f;
            }
          if (i-idx!=n)
            {
            cout<<"1881: idx="<<idx<<"; i="<<i<<"; n="<<n<<endl;
            }
          evdat.num_1881=m;
          evdat.gsum_1881=gsum;
          }
        idx+=n; n=subevent[idx]; idx++;
        evdat.size_1877=n;
          {
          //cout << "event "<<evid<<"; typ 1877"<< endl;
          //for (int x=idx; x<idx+n; x++) cout << hex << subevent[x] << endl;
          // header: adresse<31:27> wordcount<10:0>
          //  datenworte adresse<31:27>
          // header
          //  ...
          int i=idx, g, h, m=0;
          while (i-idx<n)
            {
            h=subevent[i];
            m++;
            g=(h>>27)&0x1f;
            evdat.fbslots[g]=1;
            if ((g!=16) && ((g<20) || (g>22)))
              {
              cout << "1877: g=" << g << endl;
              }
            i+=h&0x7ff;
            }
          if (i-idx!=n)
            {
            cout<<"1877: idx="<<idx<<"; i="<<i<<"; n="<<n<<endl;
            }
          evdat.num_1877=m;
          }
        idx+=n; n=subevent[idx]; idx++;
        evdat.size_1875=n;
          {
          //  datenworte adresse<31:27>
          //  ...
          int i, g, gsum=0, lastg=0, m=0;
          for (i=idx; i<idx+n; i++)
            {
            g=(subevent[i]>>27)&0x1f;
            evdat.fbslots[g]=1;
            if ((g<9) || (g>14))
              {
              cout << "1875: g=" << g << endl;
              }
            if (g!=lastg) {m++; gsum+=g;}
            lastg=g;
            }
          evdat.num_1875=m;
          evdat.gsum_1875=gsum;
          }
        idx+=n; n=subevent[idx]; idx++;
        evdat.size_1879=n;
          {
          //  datenworte
          //  ...
          //
          int i, g, lastg=0, m=0;
          for (i=idx; i<idx+n; i++)
            {
            g=(subevent[i]>>27)&0x1f;
            evdat.fbslots[g]=1;
            if ((g<17) || (g>19))
              {
              cout << "1879: g=" << g << endl;
              }
            if (g!=lastg) m++;
            lastg=g;
            }
          evdat.num_1879=m;
          }
        //if (evdat.sevsize[1]>0) good=0;
        //if (evdat.sevsize[2]>0) good=0;
        if (evdat.size_1879>0) good=0;
        if (evdat.size_1877>0) good=0;
        //if (evdat.size_1875>0) good=0; // gibt es nicht
        //if (evdat.size_1881>0) good=0; // gibt es nicht
        //if (evdat.num_1879>0) good=0;
        //if (evdat.num_1877>0) good=0;
        //if (evdat.num_1875!=3) good=0;
        //if (evdat.num_1881!=4) good=0;
        if ((evdat.tdt<11700) || (evdat.tdt>11900)) good=0;
        }
      }
    if (evdat.tdt>100000) good=0;
    if (good)
      {
      goodevents++;
      if (dobin)
        {
        int bin=evdat.tdt/binsize;
        if (bin>=binnum) bin=binnum;
        statist[bin]++;
        statist_sevsize1[bin]+=evdat.sevsize[1];
        statist_sevsize2[bin]+=evdat.sevsize[2];
        statist_size_1881[bin]+=evdat.size_1881;
        statist_size_1877[bin]+=evdat.size_1877;
        statist_size_1875[bin]+=evdat.size_1875;
        statist_size_1879[bin]+=evdat.size_1879;
        statist_num_1881[bin]+=evdat.num_1881;
        statist_gsum_1881[bin]+=evdat.gsum_1881;
        statist_num_1877[bin]+=evdat.num_1877;
        statist_num_1875[bin]+=evdat.num_1875;
        statist_gsum_1875[bin]+=evdat.gsum_1875;
        statist_num_1879[bin]+=evdat.num_1879;
        statist_size_sum[bin]+=evdat.size_1875+evdat.size_1881;
        statist_num_sum[bin]+=evdat.num_1875+evdat.num_1881;
        for (int i=0; i<26; i++) statist_fbslots[i]+=evdat.fbslots[i];
        }
      else
        evdat.print(ofs);
      }
    evdat.clear();
    }
  }
if (dobin)
  {
  int i;
  for (i=0; i<binnum; i++)
    {
    if (statist[i]>0)
      {
      statist_sevsize1[i]/=statist[i];
      statist_sevsize2[i]/=statist[i];
      statist_size_1881[i]/=statist[i];
      statist_size_1877[i]/=statist[i];
      statist_size_1875[i]/=statist[i];
      statist_size_1879[i]/=statist[i];
      statist_num_1881[i]/=statist[i];
      statist_gsum_1881[i]/=statist[i];
      statist_num_1877[i]/=statist[i];
      statist_num_1875[i]/=statist[i];
      statist_gsum_1875[i]/=statist[i];
      statist_num_1879[i]/=statist[i];
      statist_size_sum[i]/=statist[i];
      statist_num_sum[i]/=statist[i];
      }
    }
  for (i=0; i<=binnum; i++)
    {
    ofs << i;
    ofs << ' ' << statist[i];
    ofs << ' ' << statist_sevsize1[i];
    ofs << ' ' << statist_sevsize2[i];
    ofs << ' ' << statist_size_1881[i];
    ofs << ' ' << statist_size_1877[i];
    ofs << ' ' << statist_size_1875[i];
    ofs << ' ' << statist_size_1879[i];
    ofs << ' ' << statist_num_1881[i];
    ofs << ' ' << statist_gsum_1881[i];
    ofs << ' ' << statist_num_1877[i];
    ofs << ' ' << statist_num_1875[i];
    ofs << ' ' << statist_gsum_1875[i];
    ofs << ' ' << statist_num_1879[i];
    ofs << ' ' << statist_size_sum[i];
    ofs << ' ' << statist_num_sum[i];
    ofs << ' ' << (i<26?statist_fbslots[i]:0);
    ofs << endl;
    }
  //delete[] statist;
  }
cout << "last id: " << evid << endl;
cout << "good events: " << goodevents << endl;
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
