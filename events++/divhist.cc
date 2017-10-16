/******************************************************************************
*                                                                             *
* divhist.cc                                                                   *
*                                                                             *
* created: 08.02.1998                                                         *
* last changed: 08.02.1998                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <math.h>
#include <readargs.hxx>
#include <swap_int.h>
#include "versions.hxx"
extern "C" {
#include "hbook_c.h"
}

VERSION("Jul 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: divhist.cc,v 1.4 2005/02/11 15:44:59 wuestner Exp $")
#define XVERSION

C_readargs* args;
String infile1;
String infile2;
String outfile;
int iid;
int iid1;
int iid2;
int oid1;
int oid2;

/*****************************************************************************/

void printheader(ostream& os)
{
os<<"Divides two histograms binwise."<<endl<<endl;
}

/*****************************************************************************/

int readargs()
{
args->addoption("infile1", 0, "", "input file 1", "input1");
args->addoption("infile2", 1, "", "input file 2", "input2");
args->addoption("outfile", 2, "", "output file", "output");
args->addoption("iid", "id", 0, "histogram id", "id");
args->hints(C_readargs::required, "iid", "infile1", "infile2", "outfile", 0);
if (args->interpret(1)!=0) return(-1);

infile1=args->getstringopt("infile1");
infile2=args->getstringopt("infile2");
outfile=args->getstringopt("outfile");
iid=args->getintopt("iid");

return(0);
}

/*****************************************************************************/

int readhist(char* infile, int id, int ofs)
{
int lrec=0, istat;
Hropen(1, "qwert", infile, "X", &lrec, &istat);
if (istat!=0)
  {
  cout<<"                         "<<istat<<endl;
  return -1;
  }
Hrin(id, 999999, ofs);
Hrend("qwert");
return 0;
}
/*****************************************************************************/

void writehist(char* outfile, char* opt, int id)
{
Hrput(id, outfile, opt);
}

/*****************************************************************************/
float divi(float a, float b)
{
float x;
if ((a==0) && (b==0))
  x=0;
else if (b==0)
  x=0;
else
  x=a/b;
return x;
}
/*****************************************************************************/
float err(float a, float b)
{
float x;
if ((a==0) && (b==0))
  x=0;
else if ((a==0) || (b==0))
  x=0;
else
  x=a/b*sqrt(1/a+1/b);
return x;
}
/*****************************************************************************/
int divide()
{
char title[81];
int nx1, ny1;
int nx2, ny2;
int nwt, loc;
float xmin, xmax, ymin, ymax;
float xbin, ybin, xmin_, ymin_;

Hgive(iid1, title, &nx1, &xmin, &xmax, &ny1, &ymin, &ymax, &nwt, &loc);
cout<<"got "<<(ny1?2:1)<<" dim. histogram \""<<title<<"\""<<endl;
cout<<"X: nr. of bins="<<nx1<<"; min="<<xmin<<"; max="<<xmax<<endl;
if (ny1) cout<<"Y: nr. of bins="<<ny1<<"; min="<<ymin<<"; max="<<ymax<<endl;
Hgive(iid2, title, &nx2, &xmin, &xmax, &ny2, &ymin, &ymax, &nwt, &loc);
cout<<"got "<<(ny2?2:1)<<" dim. histogram \""<<title<<"\""<<endl;
cout<<"X: nr. of bins="<<nx2<<"; min="<<xmin<<"; max="<<xmax<<endl;
if (ny2) cout<<"Y: nr. of bins="<<ny2<<"; min="<<ymin<<"; max="<<ymax<<endl;

if ((nx1!=nx2) || (ny1!=ny2))
  {
  cerr<<"Dimensions do not match."<<endl;
  return -1;
  }

xbin=(xmax-xmin)/nx1;    // Binbreite
xmin_=xmin+xbin/2.;     // Mitte des ersten Bins
if (ny1>0)
  {
  ybin=(ymax-ymin)/ny1;
  ymin_=ymin+ybin/2.;
  }

if (ny1==0)
  {
  Hbook1(oid1, title, nx1, xmin, xmax, 0);
  Hbook1(oid2, "errors", nx1, xmin, xmax, 0);
  for (int ix=0; ix<nx1; ix++)
    {
    float x=xmin_+xbin*ix;
    float w1=Hi(iid1, ix+1);
    float w2=Hi(iid2, ix+1);
    Hfill(oid1, x, 0., divi(w1, w2));
    Hfill(oid2, x, 0., 1);
    }
  }
else
  {
  Hbook2(oid1, title, nx1, xmin, xmax, ny2, ymin, ymax, 0);
  Hbook2(oid2, "errors", nx1, xmin, xmax, ny2, ymin, ymax, 0);
  float asum=0, bsum=0;
  int ix, iy;
  for (iy=0; iy<ny1; iy++)
    {
    for (ix=0; ix<nx1; ix++)
      {
      asum+=Hij(iid1, ix+1, iy+1);
      bsum+=Hij(iid2, ix+1, iy+1);
      }
    }
  float fak=asum/bsum;
  cout<<"asum="<<asum<<"; bsum="<<bsum<<endl;
  cout<<"fak="<<fak<<endl;
  for (iy=0; iy<ny1; iy++)
    {
    for (ix=0; ix<nx1; ix++)
      {
      float x=xmin_+xbin*ix;
      float y=ymin_+ybin*iy;
      float w1=Hij(iid1, ix+1, iy+1);
      float w2=Hij(iid2, ix+1, iy+1);
      float q=divi(w1, w2*fak);
      float e=err(w1, w2)/fak;
      if ((ix>=3) && (ix<=5) && (iy>=3) && (iy<=5))
        {
        cout<<"["<<ix<<", "<<iy<<"] w1="<<w1<<"; w2="<<w2<<"; q="<<q<<"; e="<<e<<endl;
        }
      Hfill(oid1, x, y, q);
      Hfill(oid2, x, y, e);
      }
    }
  }
// Hgive(oid1, title, &nx2, &xmin, &xmax, &ny2, &ymin, &ymax, &nwt, &loc);
// cout<<"got "<<(ny2?2:1)<<" dim. histogram \""<<title<<"\""<<endl;
// cout<<"X: nr. of bins="<<nx2<<"; min="<<xmin<<"; max="<<xmax<<endl;
// if (ny2) cout<<"Y: nr. of bins="<<ny2<<"; min="<<ymin<<"; max="<<ymax<<endl;
// 
// for (int iy=0; iy<ny1; iy++)
//   for (int ix=0; ix<nx1; ix++)
//     {
//     cout<<"h["<<ix<<", "<<iy<<"]="<<Hij(oid1, ix+1, iy+1)<<endl;
//     }
return 0;
}
/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

initHbook();

iid1=iid+2;
iid2=iid+3;
oid1=iid;
oid2=iid+1;
if (readhist(infile1, iid, iid1-iid)!=0) return 1;
if (readhist(infile2, iid, iid2-iid)!=0) return 1;

if (divide()==0)
  {
  writehist(outfile, "N", oid1);
  writehist(outfile, "U", oid2);
  }
return 0;
}

/*****************************************************************************/
/*****************************************************************************/
