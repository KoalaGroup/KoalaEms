/******************************************************************************
*                                                                             *
* cl2hist.cc                                                                  *
*                                                                             *
* created: 22.07.1997                                                         *
* last changed: 22.07.1997                                                    *
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
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <readargs.hxx>
#include <swap_int.h>
#include <math.h>
extern "C" {
#include "hbook_c.h"
}
#include "gnuthrow.hxx"

typedef enum {clusterty_events=0, clusterty_text,
    clusterty_no_more_data=0x10000000} clustertypes;

C_readargs* args;
STRING infile, outfile;
int recsize, vednum;
int inf, itape;
int sevid, evnum, ev_id, lasthist=0, binnum;
int num_events=0, ok_events=0, rej_events=0;
int hists_ok;
float maxtime;
int *flagvals;
int *flagids;
int numflags;
struct flagcount
  {
  struct pair
    {
    int val, count;
    };
  pair vals[10];
  int num;
  };
flagcount flagcounts[5];
float tsum=0;
float tqsum=0;
// cluster[0]: size (number of following words)
// cluster[1]: endientest =0x12345678
// cluster[2]: cluster type (event=0; text=1; last cluster=0x10000000)
// cluster[3]: flags
// cluster[4]: ved_id
// cluster[5]: fragment id
// cluster[6]: number of events

/*****************************************************************************/

void printheader(ostream& os)
{
os<<"Converts Syncinfo into histograms."<<endl<<endl;
}

/*****************************************************************************/

int readargs()
{
args->helpinset(3, printheader);
args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");
args->addoption("recsize", "recsize", 65536, "maximum record size (tape only)",
    "recsize");
args->addoption("vednum", "vednum", 1, "number of VEDs", "vednum");
args->addoption("sevid", "sevid", 1, "id of subevent", "sevid");
args->addoption("evnum", "evnum", 0, "number of events", "evnum");
//args->addoption("binnum", "binnum", 1000, "number of bins", "binnum");
args->addoption("maxtime", "maxtime", (float)1000, "max. time/us", "maxtime");
args->addoption("flagid", "flagid", 0, "flagid", "flagid");
args->addoption("flagval", "flagval", -1, "flagval", "flagval");
args->multiplicity("flagid", 0);
args->multiplicity("flagval", 0);

if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
recsize=args->getintopt("recsize");
vednum=args->getintopt("vednum");
sevid=args->getintopt("sevid");
evnum=args->getintopt("evnum");
//binnum=args->getintopt("binnum");
maxtime=args->getfloatopt("maxtime");

int mi=args->multiplicity("flagid");
numflags=args->multiplicity("flagval");
flagids =new int[numflags];
flagvals=new int[numflags];

int i;
for (i=0; i<numflags; i++) flagvals[i]=args->getintopt("flagval", i);
for (i=0; i<mi; i++) flagids[i]=args->getintopt("flagid", i);
for (; i<numflags; i++) flagids[i]=i?flagids[i-1]:0;
//clog<<"flags:";
//for (i=0; i<numflags; i++) clog<<" ["<<flagids[i]<<"]: "<<flagvals[i]<<endl;
return(0);
}

/*****************************************************************************/

void init_flagcounts()
{
for (int i=0; i<5; i++)
  {
  flagcounts[i].num=0;
  }
}

/*****************************************************************************/

void print_flagcounts()
{
for (int f=0; f<5; f++)
  {
  flagcount* count=flagcounts+f;
  clog<<"flag "<<f<<":"<<endl;
  for (int i=0; i<count->num; i++)
    {
    clog<<"  "<<count->vals[i].val<<": "<<count->vals[i].count<<endl;
    }
  }
}

/*****************************************************************************/

void count_flag(int f, int v)
{
int i=0;
flagcount* count=flagcounts+f;
while ((i<count->num) && (v!=count->vals[i].val)) i++;
if (i<count->num)
  {
  count->vals[i].count++;
  }
else if (i<10)
  {
  count->num++;
  count->vals[i].val=v;
  count->vals[i].count=1;
  }
else
  {
  clog<<"flag "<<f<<": v="<<v<<"; num="<<count->num<<endl;
  }
}

/*****************************************************************************/

int do_open()
{
struct mtop mt_com;
mt_com.mt_op=MTNOP;
mt_com.mt_count=0;

if (infile=="-")
  inf=0;
else
  {
#ifdef HAVE_STRING_H
  inf=open((char*)infile, O_RDONLY, 0);
#else
  inf=open(infile.c_str(), O_RDONLY, 0);
#endif
  if (inf<0)
    {
    clog<<"open "<<infile<<" for reading: "<<strerror(errno)<<endl;
    return -1;
    }
  }
itape=ioctl(inf, MTIOCTOP, &mt_com)==0;

return 0;
}

/*****************************************************************************/

int do_close()
{
if (inf>0) close(inf);
#ifdef HAVE_STRING_H
Hrput(0, (char*)outfile, "NT");
#else
Hrput(0, outfile.c_str(), "NT");
#endif
return 0;
}

/*****************************************************************************/

// void write_data(int sev_id, unsigned int* buf, int size)
// {
// if (sev_id!=sevid) return;
// for (int i=0; i<size; i++)
//   {
//   int histid=(i+1)*10;
//   char s[100]; sprintf(s, "D%d", i);
//   if (histid>lasthist)
//     {
//     Hbook1(histid, s, 1000, 0.0, 10.0, 0.0);
//     lasthist=histid;
//     }
//   Hfill(histid, float(buf[i])/10.0, 0.0, 1.0);
//   }
// }

/*****************************************************************************/

void hbook1(int id, char* name, int binnum, float max)
{
float binsize=max/binnum;
float start=-binsize/2;
float end=max+binsize/2;
Hbook1(id, name, binnum+1, start, end, 0.0);
Hidopt(id, "STAT");
}

/*****************************************************************************/

create_hist(int initial, int s=0, int u=0)
{
float a, b, c;
binnum=10000;

if (initial)
  {
  hbook1(10, "tmc", binnum, maxtime);
  hbook1(20, "tdt", binnum, maxtime);
  hbook1(21, "tdt", binnum, maxtime*250);
  hbook1(30, "ldt", binnum, maxtime);
//  Hbook2(500, "irq2", binnum/10, a, c, binnum/10, a, c, 0.0);
  hbook1(600, "suspensions", 100, 1000.);
  hbook1(10000, "Rejected Triggers", 100, 1000.);
  hbook1(20000, "Trigger Gaps/us", 1024, 65536.);
  hists_ok=0;
  }
else
  {
  int n=40, i;
  char ss[100];

  for (i=0; i<s; i++)
    {
    sprintf(ss, "sys-%02d", i);
    hbook1(n, ss, binnum, maxtime);
    n+=10;
    }
  for (i=0; i<u; i++)
    {
    sprintf(ss, "usr-%02d", i);
    hbook1(n, ss, binnum, maxtime);
    n+=10;
    }
//   n=1000;
//   for (i=1; i<s+u; i++)
//     {
//     sprintf(ss, "diff-%02d-a", i);
//     Hbook1(n, ss, binnum, a, b, 0.0);
//     sprintf(ss, "diff-%02d-b", i);
//     Hbook1(n+1, ss, binnum, a, b, 0.0);
//     sprintf(ss, "diff-%02d-c", i);
//     Hbook1(n+2, ss, binnum, a, b, 0.0);
//     sprintf(ss, "diff-%02d-d", i);
//     Hbook1(n+3, ss, binnum, a, b, 0.0);
//     n+=10;
//     }
  hists_ok=1;
  }
}

/*****************************************************************************/

print_gaps(unsigned int* a)
{
clog<<"gaps: "<<hex<<a[0]<<", "<<a[1]<<", "<<a[2]<<dec<<endl;
}

/*****************************************************************************/

int write_data(int sev_id, unsigned int* buf, int size)
{
if (sev_id!=sevid) return 0;
unsigned int* p=buf+10;
int sflagnum=*p++;
unsigned int* sflags=p; p+=sflagnum;
int stimenum=*p++;
unsigned int* stimes=p; p+=stimenum;
int uflagnum=*p++;
unsigned int* uflags=p; p+=uflagnum;
int utimenum=*p++;
unsigned int* utimes=p; p+=utimenum;
int extranum=*p++;
unsigned int* extrap=p; p+=extranum;

if (!hists_ok)
  {
  clog<<"sflagnum="<<sflagnum<<"; stimenum="<<stimenum<<"; uflagnum="
      <<uflagnum<<"; utimenum="<<utimenum<<"; extranum="<<extranum<<endl;
  create_hist(0, stimenum, utimenum);
  }
int f, i, n;
//for (f=0; f<3; f++) count_flag(f, sflags[f]);
for (f=0; f<numflags; f++)
  {
  int flag, id;
  id=flagids[f];
  if (id<sflagnum)
    flag=sflags[id];
  else
    {
    id-=sflagnum;
    if (id<uflagnum)
      flag=uflags[f];
    else
      {
      id-=uflagnum;
      if (id<extranum)
        {
        flag=extrap[id];
        }
      else
        {
        clog<<"flagid "<<flagids[f]<<" too large"<<endl;
        return -1;
        }
      }
    }
  if (flagvals[f]!=flag) return 0;
  }
Hfill(600, float(extrap[1]), 0.0, 1.0); // suspensions
// float d1, d2;
// int o1=0, o2=0;
// d1=float(stimes[5]-stimes[4])/10.0;
// d2=float(utimes[0]-stimes[8])/10.0;
// if (d1<4.75)
//   o1=0;
// else
//   o1=1;
// if (d2<10.5)
//   o2=2;
// else
//   o2=3;
ok_events++;
rej_events+=buf[6];
Hfill(10000, float(buf[6]), 0.0, 1.0); // rej

for (int gidx=7; gidx<10; gidx++)
  {
  unsigned int gap=buf[gidx];
  if (!(gap&0x80000000) && (gap&0xffff))
      Hfill(20000, float(gap&0xffff)/10.0, 0.0, 1.0);
  if (gap&0x40000000) break;
  }

Hfill(10, float(buf[3])/10.0, 0.0, 1.0); // tmc
float tdt=float(buf[4])/10.0;
Hfill(20, tdt, 0.0, 1.0); // tdt
Hfill(21, tdt, 0.0, 1.0); // tdt
tsum+=tdt;
tqsum+=tdt*tdt;
Hfill(30, float(buf[5])/10.0, 0.0, 1.0); // ldt
n=40;
for (i=0; i<stimenum; i++)
  {
  Hfill(n, float(stimes[i])/10.0, 0.0, 1.0);
  n+=10;
  }
for (i=0; i<utimenum; i++)
  {
  Hfill(n, float(utimes[i])/10.0, 0.0, 1.0);
  n+=10;
  }
//n=1000;
// Hfill(n+o1, float(stimes[1])/10.0-float(stimes[0])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[2])/10.0-float(stimes[1])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[3])/10.0-float(stimes[2])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[4])/10.0-float(stimes[3])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[5])/10.0-float(stimes[4])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[6])/10.0-float(stimes[5])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[7])/10.0-float(stimes[6])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[8])/10.0-float(stimes[7])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(utimes[0])/10.0-float(stimes[8])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(utimes[1])/10.0-float(utimes[0])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[9])/10.0-float(utimes[1])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(stimes[10])/10.0-float(stimes[9])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(utimes[2])/10.0-float(stimes[10])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(utimes[3])/10.0-float(utimes[2])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(utimes[4])/10.0-float(utimes[3])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(utimes[5])/10.0-float(utimes[4])/10.0, 0.0, 1.0); n+=10;
// Hfill(n+o1, float(utimes[6])/10.0-float(utimes[5])/10.0, 0.0, 1.0); n+=10;
// if (o1==0)
//   {
//   n=1000;
//   Hfill(n+o2, float(stimes[1])/10.0-float(stimes[0])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[2])/10.0-float(stimes[1])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[3])/10.0-float(stimes[2])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[4])/10.0-float(stimes[3])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[5])/10.0-float(stimes[4])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[6])/10.0-float(stimes[5])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[7])/10.0-float(stimes[6])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[8])/10.0-float(stimes[7])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(utimes[0])/10.0-float(stimes[8])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(utimes[1])/10.0-float(utimes[0])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[9])/10.0-float(utimes[1])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(stimes[10])/10.0-float(stimes[9])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(utimes[2])/10.0-float(stimes[10])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(utimes[3])/10.0-float(utimes[2])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(utimes[4])/10.0-float(utimes[3])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(utimes[5])/10.0-float(utimes[4])/10.0, 0.0, 1.0); n+=10;
//   Hfill(n+o2, float(utimes[6])/10.0-float(utimes[5])/10.0, 0.0, 1.0); n+=10;
//   }
//Hfill(500, float(stimes[0])/10.0, float(stimes[6])/10.0-float(stimes[0])/10.0, 1.0);
return 0;
}

/*****************************************************************************/

int write_ev(unsigned int* buf, unsigned int size)
{
ev_id=buf[1];
if (ev_id<2) return 0;
if ((evnum!=0) && (num_events>=evnum)) return 1;
int sevnum=buf[3];
buf+=4;
size-=4;
for (int sev=0; sev<sevnum; sev++)
  {
  if (write_data(buf[0], buf+2, buf[1])<0) return -1;
  size-=buf[1]+2;
  buf+=buf[1]+2;
  }
num_events++;
if (num_events%100000==0) clog<<num_events<<endl;
return 0;
}

/*****************************************************************************/

int write_events(unsigned int* buf, unsigned int size)
{
int res;
int evnum=buf[3];
buf+=4;
size-=4;
for (int ev=0; ev<evnum; ev++)
  {
  res=write_ev(buf, size);
  if (res!=0) return res;
  size-=buf[0]+1;
  buf+=buf[0]+1;
  }
return 0;
}

/*****************************************************************************/

int do_convert_tape()
{
clog<<infile<<" is tape; not jet supported"<<endl;
return -1;
}

/*****************************************************************************/

int do_convert_file()
{
unsigned int endian=0;
unsigned int *buf=0, bsize=0, kopf[2], size, type;
int cl_idx=0, res, wenden;

do
  {
  // header lesen
  if ((res=read(inf, (char*)kopf, 2*sizeof(int)))!=2*sizeof(int))
    {
    cerr << "read new cluster: res="<<res<<": "<<strerror(errno)<<endl;
    goto fehler;
    }
  if (endian!=kopf[1])
    {
    if (endian==0)
      {
      endian=kopf[1];
      switch (endian)
        {
        case 0x12345678: wenden=0; break;
        case 0x87654321: wenden=1; break;
        default:
          clog<<hex<<"wrong endian "<<kopf[1]<<" in first cluster "
              <<"; must be 0x12345678 or 0x87654321"<<endl<<dec;
          goto fehler;
        }
      }
    else
      {
      clog<<hex<<"wrong endian "<<kopf[1]<<" in cluster "<<cl_idx
          <<"; must be "<<endian<<endl<<dec;
      goto fehler;
      }
    }
  // endien testen
  size=wenden?swap_int(kopf[0]):kopf[0];
  size--; // endientest ist schon gelesen
  // buffer allozieren
  if (bsize<size)
    {
    delete[] buf;
    buf=new unsigned int[size];
    bsize=size;
    if (buf==0)
      {
      clog<<"allocate buffer for cluster "<<cl_idx<<" ("<<size<<" words): "
          <<strerror(errno)<<endl;
      goto fehler;
      }
    //clog<<"new buf: "<<size<<" words"<<endl;
    }
  // daten lesen
  if ((res=read(inf, (char*)buf, size*sizeof(int)))!=size*sizeof(int))
    {
    cerr << "read cluster data: res="<<res<<": "<<strerror(errno)<<endl;
    goto fehler;
    }
  // endien korrigieren
  if (wenden)
    {
    for (int i=0; i<size; i++) buf[i]=swap_int(buf[i]);
    }
  switch (type=buf[0]/*cluster type*/)
    {
    case clusterty_events:
      if (cl_idx>0)
        {
        res=write_events(buf+1, size-1);
        if (res<0) goto fehler;
        if (res>0) type=clusterty_no_more_data;
        }
      break;
    case clusterty_text:
    case clusterty_no_more_data:
      break;
    default:
      clog<<"unknown cluster type "<<type<<endl;
      goto fehler;
    }
  cl_idx++;
  }
while (type!=clusterty_no_more_data);
clog<<cl_idx<<" cluster converted ("<<ok_events<<" of "<<num_events
    <<" events)"<<endl;
//print_flagcounts();
float tdtmean, tdtsigma;
tdtmean=tsum/ok_events;
tdtsigma=sqrt(tqsum/ok_events-tdtmean*tdtmean);
//clog<<"tdt sum="<<tsum<<endl;
//clog<<"tdt qsum="<<tqsum<<endl;
clog<<"tdt="<<tdtmean<<"; rejected events="<<rej_events<<endl;
return 0;
fehler:
return -1;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);
if (do_open()<0) {do_close(); return 1;}
init_flagcounts();
initHbook();
create_hist(1);
if (itape)
  do_convert_tape();
else
  do_convert_file();
do_close();
return 0;
}

/*****************************************************************************/
/*****************************************************************************/
