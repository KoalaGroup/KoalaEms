/*
 * dtmc.cc
 */

/* $ZEL: dtmc.cc,v 1.3 2004/11/26 22:20:35 wuestner Exp $ */

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>

#include <TROOT.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TF2.h>
#include <TCanvas.h>
#include <TTimer.h>
#include <TApplication.h>
#include <TSystem.h>
#include <TPaveLabel.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TObjectTable.h>
#include <versions.hxx>

VERSION("Mar 27 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: dtmc.cc,v 1.3 2004/11/26 22:20:35 wuestner Exp $")
#define XVERSION

extern void InitGui();
VoidFuncPtr_t initfuncs[] = { InitGui, 0 };
TROOT GlobalRoot("Wurzel", "This is to make ROOT happy", initfuncs);
int Error;  // needed by Motif

C_readargs* args;
float hmax, rate, dt;
double cdt1[1001];
double cdt2[1001];
double we, wr;
int num;
TCanvas* canvas;
TPad **pads;
int npads, padid;
TH1F *eh0, *eh1, *eh2, *eh3, *rh, *rh1;
TH2F *dh;

/*****************************************************************************/

int readargs()
{
int res;
//args->addoption("hmax", "hmax", (float)1000., "hmax");
args->addoption("rate", "rate", (float).01, "rate");
args->addoption("dt", "dt", (float)100., "dt");
args->addoption("num", "num", 100000, "num");

if ((res=args->interpret(1))!=0) return res;

//hmax=args->getfloatopt("hmax");
hmax=500;
rate=args->getfloatopt("rate");
dt=args->getfloatopt("dt");
num=args->getintopt("num");

return 0;
}

/*****************************************************************************/

int make_canvas(int x, int y)
{
gStyle->SetCanvasColor(10);
gStyle->SetPadColor(10);
if ((canvas=new TCanvas("Dtmc", "Dtmc", 300*x, 300*y+100))==0) return -1;
canvas->Divide(x, y, .02, .02, 0);
npads=x*y;
pads=new TPad*[npads];
int i=0;
for (int ix=0; ix<x; ix++)
  for (int iy=0; iy<y; iy++)
    {
    canvas->cd(i+1);
    pads[i]=(TPad*)gPad;
    i++;
    }
return 0;
}

/*****************************************************************************/
int make_ehist0(int n)
{
pads[n]->cd();
eh0=new TH1F("ehist0", "e0", 200, 0., hmax);
if (eh0==0) {cout<<"eh0=0"<<endl; return -1;}
eh0->Draw();
return 0;
}
/*****************************************************************************/
int make_ehist1(int n)
{
pads[n]->cd();
eh1=new TH1F("ehist1", "e1", 500, 0., 2*hmax);
if (eh1==0) {cout<<"eh1=0"<<endl; return -1;}
eh1->Draw();
return 0;
}
/*****************************************************************************/
int make_ehist2(int n)
{
pads[n]->cd();
eh2=new TH1F("ehist2", "e2", 501, -.5, 500.5);
if (eh2==0) {cout<<"eh2=0"<<endl; return -1;}
eh2->Draw();
return 0;
}
/*****************************************************************************/
int make_ehist3(int n)
{
pads[n]->cd();
eh3=new TH1F("ehist3", "e3", 501, -.5, 500.5);
if (eh3==0) {cout<<"eh3=0"<<endl; return -1;}
eh3->Draw();
return 0;
}
/*****************************************************************************/
int make_hists()
{
padid=0;

if (make_ehist0(padid++)!=0) return -1;
if (make_ehist1(padid++)!=0) return -1;
//if (make_ehist2(padid++)!=0) return -1;
//if (make_ehist3(padid++)!=0) return -1;
return 0;
}

/*****************************************************************************/
void fill()
{
static int was2=0, was3=0;
static double last2=0., last3=0.;
double h0, rf0;

  {
  int r=random();
  if (r==0) return;
  rf0=(double)r/double(0x7fffffff)*rate;
  h0=(log(rate)-log(rf0))/rate;
  }

eh0->Fill(h0, we);

// for (int i=0; i<501; i++)
//   {
//   cdt1[i]+=h0;
//   if (cdt1[i]<=i)
//     eh2->Fill(i, wr);
//   else
//     cdt1[i]=0;
//   }

last2+=h0;
if (++was2==2)
  {
  eh1->Fill(last2, we);
//   for (int i=0; i<501; i++)
//     {
//     cdt2[i]+=last2;
//     if (cdt2[i]<=i)
//       eh3->Fill(i, wr);
//     else
//       cdt2[i]=0;
//     }
  last2=0.;
  was2=0;
  }
}

/*****************************************************************************/
Double_t dead(Double_t *x, Double_t *par)
{
Double_t t=x[0], N=x[1];
Double_t r=par[0];
Double_t f;

f=pow(t, N-1)*pow(r, N)/exp(lgamma(N))*exp(-r*t);
return f;
}
/*****************************************************************************/
void plot_func(int n)
{
pads[n]->cd();
Double_t par[]={.01};
TF2 *f2 = new TF2("deadfunc",dead,0.,500.,0.,10.,1);
f2->SetNpx(501);
f2->SetNpy(101);
f2->SetParameters(par);
f2->SetParNames("rate");
f2->Draw("surf2");
}
/*****************************************************************************/

main(int argc, char* argv[])
{
TApplication app("dtmc", &argc, argv);

args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);
if (make_canvas(1, 2)<0)
  {
  cout<<"root problem"<<endl;
  return 1;
  }
// plot_func(padid++);
if (make_hists()!=0) return 1;
srandom(17);

we=rate/num*100; wr=1./num*100;
for (int i=0; i<1001; i++) {cdt1[i]=0; cdt2[i]=0;}
int n=0;
while (n<num)
  {
  for (int i=0; i<1000; i++, n++)
    {
    fill();
    }
  cerr<<n<<endl;
  for (int n=0; n<npads; n++) pads[n]->Modified();
  canvas->Update();
  }
for (n=0; n<npads; n++) pads[n]->Modified();
canvas->Update();

app.Run(kTRUE);
}

/*****************************************************************************/

