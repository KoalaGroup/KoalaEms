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
int in_id, tmp_id, add;
float x0, m;

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
args->addoption("add", "add", false, "add", "add");
args->addoption("x0", "x0", (float)95., "x0", "x0");
args->addoption("m", "m", (float)-.5, "m", "m");
args->hints(C_readargs::required, "in_id", "infile", "outfile", 0);
if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
in_id=args->getintopt("in_id");
tmp_id=in_id+1;
add=args->getboolopt("add");
x0=args->getfloatopt("x0");
m=args->getfloatopt("m");
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
Hrin(in_id, 999999, 1);
Hrend("qwert");
return 0;
}

/*****************************************************************************/

void writehist(char* outfile)
{
Hrput(in_id, outfile, "N");
}

/*****************************************************************************/

int rebin()
{
char title[81];
int nx, nnx, ny, nny, nwt, loc;
float xmin, xmin_, xmax, ymin, ymin_, ymax;
float xbin, ybin, nxmax, nymax;

Hgive(tmp_id, title, &nx, &xmin, &xmax, &ny, &ymin, &ymax, &nwt, &loc);
cout<<"got "<<(ny?2:1)<<" dim. histogram \""<<title<<"\""<<endl;
cout<<"X: nr. of bins="<<nx<<"; min="<<xmin<<"; max="<<xmax<<endl;
if (ny) cout<<"Y: nr. of bins="<<ny<<"; min="<<ymin<<"; max="<<ymax<<endl;
if (!ny) return -1;
xbin=(xmax-xmin)/nx;    // Binbreite
xmin_=xmin+xbin/2.;     // Mitte des ersten Bins
ybin=(ymax-ymin)/ny;
ymin_=ymin+ybin/2.;

if (ny)
  {
  if (add)
    Hbook1(in_id, title, 100, -20, 20, 0);
  else
    Hbook2(in_id, title, 100, -20, 20, ny, ymin, ymax, 0);
  for (int iy=0; iy<ny; iy++)
    {
    float y=ymin_+ybin*iy;
    float xoffs=x0+m*y;
    for (int ix=0; ix<nx; ix++)
      {
      float x=xmin_+xbin*ix;
      float w=Hij(tmp_id, ix+1, iy+1);
      Hfill(in_id, x-xoffs, y, w);
      }
    }
  }
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

initHbook();

if (readhist(infile)==0)
  {
  if (rebin()>=0) writehist(outfile);
  }
return 0;
}

/*****************************************************************************/
/*****************************************************************************/

