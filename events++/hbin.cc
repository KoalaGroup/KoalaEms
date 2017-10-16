/******************************************************************************
*                                                                             *
* hbin.cc                                                                     *
*                                                                             *
* created: 09.09.1997                                                         *
* last changed: 09.09.1997                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <readargs.hxx>
//#include <swap_int.h>
#include <math.h>
extern "C" {
#include "hbook_c.h"
}
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: hbin.cc,v 1.5 2005/02/11 15:45:08 wuestner Exp $")
#define XVERSION

C_readargs* args;
STRING infile, outfile;
int offs;
int multi;
int* ids;
int xre, yre;
int flip, nosub;
int listonly;
const char* dir;

/*****************************************************************************/

void printheader(ostream& os)
{
os<<"Modifies hbooks."<<endl<<endl;
}

/*****************************************************************************/
/*

hbin <infile> [<outfile>] {-iid <id>} [-offs <offs>] [-xre <v>] [-yre <v>]
  [-flip]

*/
int readargs()
{
args->addoption("infile", 0, "-", "input file", "input");
args->addoption("outfile", 1, "-", "output file", "output");
args->addoption("iid", "iid", 0, "input histogram id", "in_id");
args->multiplicity("iid", 0);
args->addoption("offs", "offs", 0, "offset for histogram ids", "offs");
args->addoption("xre", "xrebin", 1, "rebin factor for x", "xrebin");
args->addoption("yre", "yrebin", 1, "rebin factor for y", "yrebin");
args->addoption("flip", "flip", false, "flip axes", "flip");
args->addoption("nosub", "nosub", false, "ignore subdirs", "nosub");
args->addoption("dir", "dir", "", "directory in inpufile", "dir");
args->hints(C_readargs::demands, "xre", "outfile", 0);
args->hints(C_readargs::demands, "yre", "outfile", 0);
args->hints(C_readargs::demands, "flip", "outfile", 0);
if (args->interpret(1)!=0) return(-1);

infile=args->getstringopt("infile");
outfile=args->getstringopt("outfile");
xre=args->getintopt("xre");
yre=args->getintopt("yre");
flip=args->getboolopt("flip");
nosub=args->getboolopt("nosub");
offs=args->getintopt("offs");
if (args->isdefault("dir"))
  dir=0;
else
  dir=args->getstringopt("dir");

multi=args->multiplicity("iid");
if (multi>0)
  {
  ids=new int[multi];
  for (int i=0; i<multi; i++) ids[i]=args->getintopt("iid", i);
  }
else
  ids=0;
listonly=args->isdefault("offs") && args->isdefault("xre")
    && args->isdefault("yre") && args->isdefault("flip");
return(0);
}

/*****************************************************************************/

int openinfile(const char* infile)
{
int lrec=0, istat;
Hropen(1, "LUN1", infile, "X", &lrec, &istat);
if (istat!=0)
  {
  cout<<"                         "<<istat<<" ("<<strerror(istat)<<")"<<endl;
  return -1;
  }
return 0;
}

/*****************************************************************************/

int openoutfile(const char* outfile)
{
int lrec=1024, istat;
Hropen(2, "LUN2", outfile, "NX", &lrec, &istat);
if (istat!=0)
  {
  cout<<"                         "<<istat<<" ("<<strerror(istat)<<")"<<endl;
  return -1;
  }
return 0;
}

/*****************************************************************************/

int closeinfile()
{
Hrend("LUN1");
return 0;
}

/*****************************************************************************/

int closeoutfile()
{
Hrend("LUN2");
return 0;
}

/*****************************************************************************/

int printhist(int id)
{
Hrin(id, 999999, 0);

char title[HLen];
int nx, nnx, ny, nny, nwt, loc;
float xmin, xmin_, xmax, ymin, ymin_, ymax;
float oxbin, oybin, nxbin, nybin, nxmax, nymax; 
Hgive(id, title, &nx, &xmin, &xmax, &ny, &ymin, &ymax, &nwt, &loc);
if (ny==0)
  {
  cout<<"    "<<nx<<" bins ("<<xmin<<":"<<xmax<<")"<<endl;
  }
else
  {
  cout<<"    x: "<<nx<<" bins ("<<xmin<<":"<<xmax<<")"<<endl;
  cout<<"    y: "<<ny<<" bins ("<<ymin<<":"<<ymax<<")"<<endl;
  }
Hdelet(id);
return 0;
}

/*****************************************************************************/

int listdir(int level)
{
if (level>2) return 0;
char path[HLen], opath[HLen];
char chtype[HLen], chtitl[HLen];

Hcdir(opath, "R");
cout<<"current directory: "<<path<<endl;

int idh=0;
Hlnext(&idh, chtype, chtitl, " ");
while (idh)
  {
  cout<<idh<<": \""<<chtitl<<"\"; ";
  switch (chtype[0])
    {
    case '1':
      cout<<"Histogram 1 D"<<endl;
      printhist(idh);
      break;
    case '2':
      cout<<"Histogram 2 D"<<endl;
      printhist(idh);
      break;
    case 'N':
      cout<<"Ntuple"<<endl;
      break;
    case 'D':
      cout<<"Directory"<<endl;
      if (!nosub)
        {
        strncpy(path, chtitl, HLen);
        Hcdir(path,  " ");
        listdir(level+1);
        Hcdir(opath, " ");
        Hcdir(path, "R");
        cout<<"wieder in: "<<path<<endl;
        }
      break;
    case '?':
    default:
      cout<<"unknown code"<<endl;
    }
    
  Hlnext(&idh, chtype, chtitl, " ");
  }

return 0;
}

/*****************************************************************************/

int list()
{
char path[HLen];

strcpy(path, "//LUN1");
Hcdir(path, " ");
if (dir!=0)
  {
  strncpy(path, dir, HLen);
  Hcdir(path, " ");
  }
if (multi)
  {
  for (int n=0; n<multi; n++) printhist(ids[n]);
  }
else
  listdir(0);
return 0;
}

/*****************************************************************************/

int rebin(int iid, int oid)
{
char title[HLen];
int nx, nnx, ny, nny, nwt, loc;
float xmin, xmin_, xmax, ymin, ymin_, ymax;
float oxbin, oybin, nxbin, nybin, nxmax, nymax;

Hgive(iid, title, &nx, &xmin, &xmax, &ny, &ymin, &ymax, &nwt, &loc);
//cout<<"got "<<(ny?2:1)<<" dim. histogram \""<<title<<"\""<<endl;
//cout<<"X: nr. of bins="<<nx<<"; min="<<xmin<<"; max="<<xmax<<endl;
//if (ny) cout<<"Y: nr. of bins="<<ny<<"; min="<<ymin<<"; max="<<ymax<<endl;

oxbin=(xmax-xmin)/nx;   // alte Binbreite
xmin_=xmin+oxbin/2.;    // Mitte des ersten Bins
nnx=nx/xre;             // neue Binanzahl
nxbin=oxbin*xre;        // neue Binbreite
nxmax=xmin+nxbin*nnx;   // neue obere Grenze
//printf("xbin=%f; xmin_=%f; nnx=%d\n", oxbin, xmin_, nnx);
//printf("nxbin=%f; nxmax=%f\n", nxbin, nxmax);
if (ny)
  {
  oybin=(ymax-ymin)/ny;
  ymin_=ymin+oybin/2.;
  nny=ny/yre;
  nybin=oybin*yre;
  nymax=ymin+nybin*nny;
//  printf("ybin=%f; ymin_=%f; nny=%d\n", oybin, ymin_, nny);
//  printf("nybin=%f; nymax=%f\n", nybin, nymax);
  
  if (!flip)
    Hbook2(oid, title, nnx, xmin, nxmax, nny, ymin, nymax, 0);
  else
    Hbook2(oid, title, nny, ymin, nymax, nnx, xmin, nxmax, 0);
  for (int ix=0; ix<nx; ix++)
    {
    float x=xmin_+oxbin*ix;
    for (int iy=0; iy<ny; iy++)
      {
      float y=ymin_+oybin*iy;
      float w=Hij(iid, ix+1, iy+1);
      if (!flip)
        Hfill(oid, x, y, w);
      else
        Hfill(oid, y, x, w);
      }
    }
  }
else
  {
  Hbook1(oid, title, nnx, xmin, nxmax, 0);
  for (int ix=0; ix<nx; ix++)
    {
    float x=xmin_+oxbin*ix;
    float w=Hi(iid, ix+1);
    Hfill(oid, x, 0., w);
    }
  }
return 0;
}

/*****************************************************************************/

int convert_hist(int id)
{
int iid, oid, cycle;
char path[HLen], opath[HLen];

Hrin(id, 999999, !offs);
iid=id+!offs;
oid=id+offs;

rebin(iid, oid);

Hcdir(opath, "R");
strcpy(path, "//LUN2");
Hcdir(path,  " ");

Hrout(oid, &cycle, "");
if (cycle!=1) cerr<<"Warning: Cycle "<<cycle<<" for id "<<oid<<endl;

Hcdir(opath, " ");

Hdelet(iid);
Hdelet(oid);
return 0;
}

/*****************************************************************************/

int convert()
{
char path[HLen];
strcpy(path, "//LUN1");
Hcdir(path, " ");
if (dir!=0)
  {
  strncpy(path, dir, HLen);
  Hcdir(path, " ");
  }
Hcdir(path, "R");
cerr<<"current dir is: "<<path<<endl;

for (int n=0; n<multi; n++)
  {
  cerr<<"converting "<<ids[n]<<endl;
  convert_hist(ids[n]);
  }

return 0;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

initHbook();

#ifdef HAVE_STRING_H
if (openinfile(infile)<0) return 1;
#else
if (openinfile(infile.c_str())<0) return 1;
#endif

if (listonly)
  {
  list();
  }
else
  {
#ifdef HAVE_STRING_H
  if (openoutfile(outfile)<0)
#else
  if (openoutfile(outfile.c_str())<0)
#endif
    {
    closeinfile();
    return 1;
    }
  convert();
  closeoutfile();
  }
closeinfile();
return 0;
}

/*****************************************************************************/
/*****************************************************************************/
