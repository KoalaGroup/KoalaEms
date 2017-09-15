/*
 * syncdisp2.cc          
 *                       
 * created 30.09.98 PW   
 */

#include "config.h"
#include "cxxcompat.hxx"
#include "readargs.hxx"
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <errorcodes.h>
#include <errno.h>
#include <inbuf.hxx>
#include <outbuf.hxx>
#include "client_defaults.hxx"

#include <TROOT.h>
#include <TFile.h>
#include <TH1.h>
#include <TCanvas.h>
#include <TTimer.h>
#include <TApplication.h>
#include <TSystem.h>
#include <TPaveLabel.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TObjectTable.h>
#include <versions.hxx>

VERSION("Sep 30 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: syncdisp2.cc,v 1.3 2004/11/26 22:20:51 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
class Statist
  {
  public:
    Statist() :h(0), ih(0), pad(0), ved(0) {}
    ~Statist() {delete h;}
    char name[100];
    char iname[100];
    char itname[100];
    char title[100];
    String vedname;
    int type;
    TPad* pad; 
    C_VED* ved;
    int Read(C_inbuf&);
    void MakeNames();
    void print(ostream&);
    void Draw() {if (h) h->Draw(); if (ih) ih->Draw();}
    int MakeHist(int nbin, float x0, float x1);
    struct arrdims
      {
      int operator==(arrdims&a) {return (nbin==a.nbin) && (xmax==a.xmax);}
      int operator!=(arrdims&a) {return (nbin!=a.nbin) || (xmax!=a.xmax);}
      int nbin;
      float xmax;
      };
    void getstatist();
    arrdims oldarrdim, arrdim;
    float bin_width;
    TH1F *h, *ih, *ith;
  // read from VED
    int entries, ovl;
    int min, max;
    unsigned long sum, qsum;
    int histsize, histscale;
    int leer;
  };

class TUpdateTimer: public TTimer
  {
  public:
    TUpdateTimer(int sec);
    virtual Bool_t Notify();
  };

extern void InitGui();
VoidFuncPtr_t initfuncs[] = { InitGui, 0 };
TROOT GlobalRoot("Wurzel", "This is to make ROOT happy", initfuncs);
int Error;  // needed by Motif

STRING* vednames;
Statist* statists;
TCanvas* canvas;
int numveds, numhists;
int totaldt;
C_readargs* args;

/*****************************************************************************/
int readargs()
{
int res;
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK,
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("interval", "interval", 10, "update every x seconds",
    "interval");
args->addoption("totaldt", "totaldt", false, "show total teadtime");
args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
args->hints(C_readargs::exclusive, "port", "sockname", 0);

if ((res=args->interpret(1))!=0) return res;

totaldt=args->getboolopt("totaldt");

numveds=args->numnames();
if (numveds==0)
  {
  cerr<<"at least one vedname must be given"<<endl;
  return -1;
  }
vednames=new STRING[numveds];
for (int i=0; i<numveds; i++) vednames[i]=args->getnames(i);
return 0;
}
/*****************************************************************************/
int make_canvas()
{
gStyle->SetCanvasColor(10);
gStyle->SetPadColor(10);
char wtitle[100], res_name[100];
sprintf(wtitle, "Readout Statistics");
sprintf(res_name, "syncdisplay");

int canvsize=1000;
int padsize=canvsize/numhists;
if (padsize<100) padsize=100;
padsize=numhists*padsize;

if ((canvas=new TCanvas(res_name, wtitle, 640, canvsize))==0) return -1;
canvas->SetEditable(kIsNotEditable);
canvas->SetFillColor(42);
canvas->GetFrame()->SetFillColor(21);
canvas->GetFrame()->SetBorderSize(6);
canvas->GetFrame()->SetBorderMode(-1);

for (int i=0; i<numhists; i++)
  {
  canvas->cd();
  char pname[100], hpname[100];
  sprintf(pname, "%s_%d", "histpad", i+1);
  sprintf(hpname, "%s_%d", "hhistpad", i+1);
  float x0, x1, y0, y1;
  x0=0; x1=1.;
  y0=(numhists-i-1)*(1./numhists);
  y1=(numhists-i)*(1./numhists);
  statists[i].pad=new TPad(pname, pname, x0, y0, x1, y1, 17, 0, 0);
  statists[i].pad->Draw();
  }

//gObjectTable->Print();
}
/*****************************************************************************/
int Statist::MakeHist(int nbin, float x0, float x1)
{
int need_h=1, need_ih=0, need_ith=0;
switch (type)
  {
  case 1:
    need_ih=1; need_ith=1;
    break;
  case 2:
    need_ih=1; need_ith=1;
    break;
  }
pad->cd();
if (h) delete h; h=0;
if (ih) delete ih; ih=0;
if (ith) delete ith; ith=0;
if (need_h)
  {
  h=new TH1F(name, title, arrdim.nbin, 0., arrdim.xmax);
  if (h==0) {cerr<<"new TH1F sclug fehl"<<endl; return -1;}
  h->SetXTitle("us");
  h->SetLabelFont(20, "X");
  h->SetLabelSize(0.04, "X");
  h->SetLabelFont(20, "Y");
  h->SetLabelSize(0.04, "Y");
  h->SetFillStyle(1001);
  h->SetFillColor(5);
  h->SetLineColor(1);
  h->SetLineStyle(1);
  h->SetLineWidth(3);
  h->Draw();
  }
if (need_ih)
  {
  ih=new TH1F(iname, title, arrdim.nbin, 0., arrdim.xmax);
  if (ih==0) {cerr<<"new TH1F sclug fehl"<<endl; return -1;}
  ih->SetLineColor(4);
  ih->SetLineStyle(1);
  ih->SetLineWidth(1);
  ih->Draw("same");
  }
if (need_ith)
  {
  ith=new TH1F(itname, title, arrdim.nbin, 0., arrdim.xmax);
  if (ith==0) {cerr<<"new TH1F sclug fehl"<<endl; return -1;}
  ith->SetLineColor(2);
  ith->SetLineStyle(1);
  ith->SetLineWidth(1);
  ith->Draw("same");
  }
return 0;
}
/*****************************************************************************/
void Statist::MakeNames()
{
if (type==2)
  {
  sprintf(name, "tdt_%s", (char*)vedname);
  sprintf(iname, "itdt_%s", (char*)vedname);
  sprintf(itname, "ittdt_%s", (char*)vedname);
  sprintf(title, "%s, tdt", (char*)vedname);
  }
else
  {
  sprintf(name, "ldt_%s", (char*)vedname);
  sprintf(iname, "ildt_%s", (char*)vedname);
  sprintf(itname, "itldt_%s",(char*) vedname);
  sprintf(title, "%s", (char*)vedname);
  }
}
/*****************************************************************************/
int Statist::Read(C_inbuf& ib)
{
ib++; // skip type
ib>>entries>>ovl>>min>>max>>sum>>qsum>>histsize>>histscale;
ib>>arrdim.nbin>>leer;

bin_width=0.1; // 100 ns in us
if (histscale) bin_width*=histscale;
arrdim.xmax=bin_width*arrdim.nbin;

if (!h || (oldarrdim!=arrdim))
  {
  if (MakeHist(arrdim.nbin, 0., arrdim.xmax)<0) return -1;
  oldarrdim=arrdim;
  }
int i; double summe=0; double tsumme=0; float maximum=0;
for (i=0; i<leer; i++) h->SetBinContent(i, 0);
for (; i<arrdim.nbin; i++)
  {
  float v=ib.getint();
  if (i>arrdim.nbin-10) continue;
  if (v>maximum) maximum=v;
  h->SetBinContent(i, v);
  if (ih)
    {
    summe+=v;
    ih->SetBinContent(i, summe);
    }
  if (ith)
    {
    tsumme+=v*i*bin_width;
    ith->SetBinContent(i, tsumme);
    }
  }
h->Scale(maximum>0?1./maximum:1.);
h->Modified();
if (ih)
  {
  ih->Scale(entries>0?1./(double)entries:1.);
  ih->Modified();
  }
if (ith)
  {
  ith->Scale(sum>0?10./(double)sum:10.); // 10. wegen 100 ns in us
  ith->Modified();
  }
h->GetYaxis()->SetLimits(0., 1.1);
pad->Modified();
return 0;
}
/*****************************************************************************/
void Statist::getstatist()
{
try
  {
  if (ved==0) ved=new C_VED(vedname);

  C_confirmation* conf=ved->is0()->command("GetSyncStatist",4,type,0,0,0);
  C_inbuf ib(conf->buffer(), conf->size());
  ib++; // skip errorcode
  ib++; // skip counter
  ib+=3; // skip evnum (int) and rejected (long !!);
  ib++; // skip arraycounter (must be 1)
  pad->cd();
  Read(ib);
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
}
/*****************************************************************************/
void periodic_handler()
{
int i;
for (i=0; i<numhists; i++)
  {
  statists[i].getstatist();
  }
canvas->Modified();
canvas->Update();
}
/*****************************************************************************/
TUpdateTimer::TUpdateTimer(int sec)
:TTimer(sec*1000, kFALSE)
{}
/*****************************************************************************/

virtual Bool_t TUpdateTimer::Notify()
{
periodic_handler();
Reset();
//return kTRUE;
return kFALSE;
}
/*****************************************************************************/
main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  TApplication app("SyncDisplay", 0, 0);

  if (totaldt) numhists=2*numveds; else numhists=numveds;
  statists=new Statist[numhists];
  if (totaldt)
    {
    for (int i=0; i<numveds; i++)
      {
      statists[2*i].vedname=vednames[i];
      statists[2*i].type=2;
      statists[2*i+1].vedname=vednames[i];
      statists[2*i+1].type=1;
      }
    }
  else
    {
    for (int i=0; i<numveds; i++)
      {
      statists[i].vedname=vednames[i];
      statists[i].type=1;
      }
    }
  for (int i=0; i<numhists; i++) statists[i].MakeNames();
  if (make_canvas()<0)
    {
    cout<<"root problem"<<endl;
    return 1;
    }
  try
    {
    if (!args->isdefault("hostname") || !args->isdefault("port"))
      communication.init(args->getstringopt("hostname"),
          args->getintopt("port"));
    else
      communication.init(args->getstringopt("sockname"));
    }
  catch (C_error* e)
    {
    cerr << (*e) << endl;
    delete e;
    return(1);
    }
  periodic_handler();
  TUpdateTimer* timer=new TUpdateTimer(args->getintopt("interval"));
  gSystem->AddTimer(timer);
  signal(SIGFPE, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
  app.Run(kTRUE);
  delete timer;
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
communication.done();
}

/*****************************************************************************/
/*****************************************************************************/
