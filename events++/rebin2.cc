/******************************************************************************
*                                                                             *
* rebin2.cc                                                                   *
*                                                                             *
* created: 09.09.1997                                                         *
* last changed: 09.09.1997                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <iostream.h>
#include <String.h>
#include <errno.h>
#include <readargs.hxx>
#include <swap_int.h>
#include <math.h>
extern "C" {
#include "hbook_c.h"
}

C_readargs* args;
String infile, outfile;
int in_id, out_id, tmp_id;
int multi;
int* ids;
int xre, yre;
int flip;

/*****************************************************************************/

void printheader(ostream& os)
{
os<<"Converts Syncinfo into histograms."<<endl<<endl;
}

/*****************************************************************************/

int readargs()
{
args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");
args->addoption("in_id", "iid", 0, "input histogram id", "in_id");
args->addoption("out_id", "oid", 0, "output histogram id", "out_id");
args->addoption("xre", "xrebin", 1, "rebin factor for x", "xrebin");
args->addoption("yre", "yrebin", 1, "rebin factor for y", "yrebin");
args->addoption("flip", "flip", false, "flip axes", "flip");
args->hints(C_readargs::required, "in_id", "infile", "outfile", 0);
args->multiplicity("in_id", 0);
if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
multi=args->multiplicity("in_id");
if (multi>1)
  {
  }
else
in_id=args->getintopt("in_id");
if (args->isdefault("out_id"))
  out_id=in_id;
else
  out_id=args->getintopt("out_id");
if (in_id==out_id)
  tmp_id=in_id+1;
else
  tmp_id=in_id;
xre=args->getintopt("xre");
yre=args->getintopt("yre");
flip=args->getboolopt("flip");
return(0);
}

/*****************************************************************************/

int readhist(char* infile)
{
int lrec=0, istat;
Hropen(1, "qwert", infile, "X", &lrec, &istat);
if (istat!=0)
  {
  cout<<"                         "<<istat<<" ("<<strerror(istat)<<")"<<endl;
  return -1;
  }
Hrin(in_id, 999999, tmp_id-in_id);
Hrend("qwert");
return 0;
}

/*****************************************************************************/

void writehist(char* outfile)
{
Hrput(out_id, outfile, "N");
}

/*****************************************************************************/

void rebin()
{
char title[81];
int nx, nnx, ny, nny, nwt, loc;
float xmin, xmin_, xmax, ymin, ymin_, ymax;
float oxbin, oybin, nxbin, nybin, nxmax, nymax;
float sum=0;

Hgive(tmp_id, title, &nx, &xmin, &xmax, &ny, &ymin, &ymax, &nwt, &loc);
cout<<"got "<<(ny?2:1)<<" dim. histogram \""<<title<<"\""<<endl;
cout<<"X: nr. of bins="<<nx<<"; min="<<xmin<<"; max="<<xmax<<endl;
if (ny) cout<<"Y: nr. of bins="<<ny<<"; min="<<ymin<<"; max="<<ymax<<endl;

oxbin=(xmax-xmin)/nx;   // alte Binbreite
xmin_=xmin+oxbin/2.;    // Mitte des ersten Bins
nnx=nx/xre;             // neue Binanzahl
nxbin=oxbin*xre;        // neue Binbreite
nxmax=xmin+nxbin*nnx;   // neue obere Grenze
printf("xbin=%f; xmin_=%f; nnx=%d\n", oxbin, xmin_, nnx);
printf("nxbin=%f; nxmax=%f\n", nxbin, nxmax);
if (ny)
  {
  oybin=(ymax-ymin)/ny;
  ymin_=ymin+oybin/2.;
  nny=ny/yre;
  nybin=oybin*yre;
  nymax=ymin+nybin*nny;
  printf("ybin=%f; ymin_=%f; nny=%d\n", oybin, ymin_, nny);
  printf("nybin=%f; nymax=%f\n", nybin, nymax);
  
  if (!flip)
    Hbook2(out_id, title, nnx, xmin, nxmax, nny, ymin, nymax, 0);
  else
    Hbook2(out_id, title, nny, ymin, nymax, nnx, xmin, nxmax, 0);
  for (int ix=0; ix<nx; ix++)
    {
    float x=xmin_+oxbin*ix;
    for (int iy=0; iy<ny; iy++)
      {
      float y=ymin_+oybin*iy;
      float w=Hij(tmp_id, ix+1, iy+1);
      sum+=w;
      if (!flip)
        Hfill(out_id, x, y, w);
      else
        Hfill(out_id, y, x, w);
      }
    }
  }
else
  {
  Hbook1(out_id, title, nnx, xmin, nxmax, 0);
  for (int ix=0; ix<nx; ix++)
    {
    float x=xmin_+oxbin*ix;
    float w=Hi(tmp_id, ix+1);
    sum+=w;
    Hfill(out_id, x, 0., w);
    }
  }
cout<<"sum="<<sum<<endl;
}

/*****************************************************************************/

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

initHbook();

if (readhist(infile)==0)
  {
  rebin();
  writehist(outfile);
  }
return 0;
}

/*****************************************************************************/
/*****************************************************************************/

