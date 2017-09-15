/******************************************************************************
*                                                                             *
* w2ped.cc                                                                    *
*                                                                             *
* OSF1                                                                        *
*                                                                             *
* created 24.09.95                                                            *
* last changed 24.09.95                                                       *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: searchped.cc,v 1.5 2005/02/11 15:45:11 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

STRING getdate(const STRING& prefix)
{
STRING st;
STRING ss=prefix+"/main_0";
ifstream istr(ss.c_str());
if (istr.good())
  {
  char s[1024];
  istr.getline(s, 1024);
  istr.getline(s, 1024);
  st=s;
  }
else
  {
  clog << "main_0 not good" << endl;
  st="unknown";
  }
return st;
}

/*****************************************************************************/

struct pedestal
  {
  int operator==(const pedestal&) const;
  int operator!=(const pedestal&) const;
  STRING date;
  struct channel
    {
    int ped;
    float mped;
    float dev;
    };
  channel chan[1024];
  };

int pedestal::operator==(const pedestal& ped) const
{
clog << date << "=?=" << ped.date << endl;
int res=1;
for (int i=0; res && (i<1024); i++)
  {
  if (ped.chan[i].mped!=chan[i].mped) res=0;
  if (ped.chan[i].dev!=chan[i].dev) res=0;
  }
return res;
}

int pedestal::operator!=(const pedestal& ped) const
{
clog << date << " <==> " << ped.date << endl;
int res=0;
for (int i=0; !res && (i<1024); i++)
  {
  if (ped.chan[i].mped!=chan[i].mped) res=1;
  if (ped.chan[i].dev!=chan[i].dev) res=1;
  }
return res;
}

/*****************************************************************************/

pedestal* getpedestal(const STRING& prefix)
{
pedestal* pedest;
STRING ss=prefix+"/drams_0";
ifstream istr(ss.c_str());
if (istr.good())
  {
  pedest=new pedestal;
  int chan=0;

  char line[1024];
  STRING s, s1;

  while (istr.getline(line, 1024, '\n'))
    {
    int idx;
    s=line;
#ifdef NO_ANSI_CXX
    if ((idx=s.index("Chann:"))<0) continue;
#else
    if ((idx=s.find("Chann:"))==string::npos) continue;
#endif
    ISSTREAM str(line);
    int ch, ped, imped, idev;
    float mped, dev;
    str >> s1 >> ch >> s1 >> s1 >> ped >> s1 >> mped >> s1 >> s1 >> dev;
    if ((ch!=chan+1) || (chan>=1024)) clog << "falscher Kanal" << endl;
    pedest->chan[chan].ped=ped;
    pedest->chan[chan].mped=mped;
    pedest->chan[chan].dev=dev;
    chan++;
    }
  if (chan!=1024) clog << "es fehlen Kanaele" << endl;
  pedest->date=getdate(prefix);
  clog << pedest->date << endl;
  }
else
  {
  clog << "drams_0 not good" << endl;
  pedest=0;
  }
return pedest;
}

/*****************************************************************************/

void writepedestal(pedestal* pedest)
{
clog << "write " << pedest->date << endl;
ofstream of(pedest->date.c_str());
if (of.good())
  {
  int lidx=0;
  of.setf(ios::hex, ios::basefield);
  of.setf(ios::showbase);
  for (int i=0; i<1024; i++)
    {
    int imped, idev;
    float mped, dev;
    mped=pedest->chan[i].mped;
    dev=pedest->chan[i].dev;
    imped=(int)(mped*1000);
    idev=(int)(dev*1000);
    of << setw(10) << imped << " " << setw(10) << idev;
    if ((++lidx%3)==0)
      of << endl;
    else
      of << " ";
    }
  if ((lidx%3)!=0) of << endl;
  }
else
  cerr << pedest->date << " not good" << endl;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
char* dir;
if (argc>1)
  dir=argv[1];
else
  dir=0;

int n=1;
int ok=1;
pedestal* lastped=0;
pedestal* ped;

do
  {
  OSSTREAM ss;
  if (dir)
    ss << dir << "/file_" << n << ends;
  else
    ss << "file_" << n << ends;
  clog << "file_" << n << " :" << endl;
  ped=getpedestal(ss.str());
  if (ped!=0)
    {
    if (lastped==0)
      lastped=ped;
    else if ((*ped)!=(*lastped))
      {
      writepedestal(lastped);
      delete lastped;
      lastped=ped;
      }
    }
  else
    {
    if (lastped!=0) writepedestal(lastped);
    ok=0;
    }
  n++;
  }
while (ok);

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
