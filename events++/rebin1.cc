/******************************************************************************
*                                                                             *
* rebin2.cc                                                                   *
*                                                                             *
* created: 11.09.1997                                                         *
* last changed: 11.09.1997                                                    *
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
#include <readargs.hxx>
#include <swap_int.h>
#include <math.h>
extern "C" {
#include "hbook_c.h"
}

C_readargs* args;
STRING infile, outfile;
int in_id, out_id, tmp_id;
float x_0, x_1, y_0, y_1;
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
args->addoption("x0", "x0", (float)0., "new x0", "x0");
args->addoption("y0", "y0", (float)0., "new y0", "y0");
args->addoption("x1", "x1", (float)0., "new x1", "x1");
args->addoption("y1", "y1", (float)0., "new y1", "y1");
args->addoption("flip", "flip", false, "flip axes", "flip");
args->hints(C_readargs::required, "in_id", "infile", "outfile", 0);
if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
in_id=args->getintopt("in_id");
if (args->isdefault("out_id"))
  out_id=in_id;
else
  out_id=args->getintopt("out_id");
if (in_id==out_id)
  tmp_id=in_id+1;
else
  tmp_id=in_id;
flip=args->getboolopt("flip");
return(0);
}

/*****************************************************************************/

int readhist(char* infile)
{
int lrec=0, istat;
cerr<<"Hropen("<<infile<<")"<<endl;
Hropen(1, "qwert", infile, "X", &lrec, &istat);
cerr<<"nach Hropen"<<endl;
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
float xbin, ybin, nxmax, nymax;

Hgive(tmp_id, title, &nx, &xmin, &xmax, &ny, &ymin, &ymax, &nwt, &loc);
cout<<"got "<<(ny?2:1)<<" dim. histogram \""<<title<<"\""<<endl;
cout<<"X: nr. of bins="<<nx<<"; min="<<xmin<<"; max="<<xmax<<endl;
if (ny) cout<<"Y: nr. of bins="<<ny<<"; min="<<ymin<<"; max="<<ymax<<endl;

if (args->isdefault("x0")) x_0=xmin; else x_0=args->getfloatopt("x0");
if (args->isdefault("x1")) x_1=xmax; else x_1=args->getfloatopt("x1");

xbin=(xmax-xmin)/nx;    // Binbreite
xmin_=xmin+xbin/2.;     // Mitte des ersten Bins
nnx=(x_1-x_0)/xbin;       // neue Binanzahl
if (x_0+xbin*nnx<x_1) nnx++;
x_1=x_0+xbin*nnx;
cerr<<"xbin="<<xbin<<"; xmin_="<<xmin_<<"nnx="<<nnx<<endl;
cerr<<"x0="<<x_0<<"; x1="<<x_1<<endl;
if (ny)
  {
  if (args->isdefault("y0")) y_0=ymin; else y_0=args->getfloatopt("y0");
  if (args->isdefault("y1")) y_1=ymax; else y_1=args->getfloatopt("y1");
  ybin=(ymax-ymin)/ny;
  ymin_=ymin+ybin/2.;
  nny=(y_1-y_0)/ybin;       // neue Binanzahl
  if (y_0+ybin*nny<y_1) nny++;
  y_1=y_0+ybin*nny;
  cerr<<"ybin="<<ybin<<"; ymin_"<<ymin_<<"; nny="<<nny<<endl;
  cerr<<"y0="<<y_0<<"; y1="<<y_1<<endl;
  
  if (!flip)
    Hbook2(out_id, title, nnx, x_0, x_1, nny, y_0, y_1, 0);
  else
    Hbook2(out_id, title, nny, y_0, y_1, nnx, x_0, x_1, 0);
  for (int ix=0; ix<nx; ix++)
    {
    float x=xmin_+xbin*ix;
    for (int iy=0; iy<ny; iy++)
      {
      float y=ymin_+ybin*iy;
      float w=Hij(tmp_id, ix+1, iy+1);
      if (!flip)
        Hfill(out_id, x, y, w);
      else
        Hfill(out_id, y, x, w);
      }
    }
  }
else
  {
  Hbook1(out_id, title, nnx, x_0, x_1, 0);
  for (int ix=0; ix<nx; ix++)
    {
    float x=xmin_+xbin*ix;
    float w=Hi(tmp_id, ix+1);
    Hfill(out_id, x, 0., w);
    }
  }
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

initHbook();

#ifdef HAVE_STRING_H
if (readhist(infile)==0)
#else
if (readhist(infile.c_str())==0)
#endif
  {
  rebin();
#ifdef HAVE_STRING_H
  writehist(outfile);
#else
  writehist(outfile.c_str());
#endif
  }
return 0;
}

/*****************************************************************************/
/*****************************************************************************/

