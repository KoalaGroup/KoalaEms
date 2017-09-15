/******************************************************************************
*                                                                             *
* syncdisp .cc                                                                *
*                                                                             *
* created: 20.10.97                                                           *
* last changed: 11.11.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
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

VERSION("Oct 10 1997", __FILE__, __DATE__, __TIME__,
"$ZEL: syncdisp.cc,v 1.9 2004/11/26 22:20:50 wuestner Exp $")

struct Tpads
  {
  TPad* pad;
  TPad* histpad;
  TPaveLabel* text;
  };
Tpads* pads;

class Statist
  {
  public:
    Statist() :h(0), ih(0), pad(0) {}
    ~Statist() {delete h;}
    const char* name;
    const char* iname;
    const char* itname;
    const char* title;
    int Read(C_inbuf&, int);
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
    arrdims oldarrdim, arrdim;
    float bin_width;
    int newscale;
    TH1F *h, *ih, *ith;
    Tpads* pad;
  // read from VED
    int typ;
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

C_VED* ved;
C_instr_system* is;
C_proclist* proclist;
C_readargs* args;
int verbose, debug;
int hists, nhist, ved_is_open, maxchan;
const char* vedname;
TCanvas* canvas;
TPad* histpad;
Statist* statist;
TPaveText* Events;
struct timeval last_tv;
int last_evnum;
unsigned long last_rejected;

/*****************************************************************************/

int readargs()
{
int res;
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
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
args->addoption("hists", "hists", 7, "histogram mask", "hists");
args->addoption("maxchan", "maxchan", 0, "number of channels", "maxchan");
args->addoption("vedname", 0, "", "Name des VED", "vedname");
args->hints(C_readargs::required, "vedname", 0);
args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
args->hints(C_readargs::exclusive, "port", "sockname", 0);

if ((res=args->interpret(1))!=0) return res;

debug=args->getboolopt("debug");
verbose=debug || args->getboolopt("verbose");
hists=args->getintopt("hists");
nhist=0; int m=hists;
while (m)
  {
  if (m&1) nhist++;
  m>>=1;
  }
maxchan=args->getintopt("maxchan");
vedname=args->getstringopt("vedname");
if (debug) cout<<"vedname =\""<<vedname<<"\""<<endl;
return 0;
}

/*****************************************************************************/

int open_ved()
{
try
  {
  if (debug) cout<<"try communication.init"<<endl;
  if (!args->isdefault("hostname") || !args->isdefault("port"))
    {
    if (debug) cout << "use host \"" << args->getstringopt("hostname")
        << "\", port " << args->getintopt("port") << endl;
    communication.init(args->getstringopt("hostname"),
        args->getintopt("port"));
    }
  else
    {
    if (debug) cout << "use socket \""
        << args->getstringopt("sockname") << "\"" << endl;
    communication.init(args->getstringopt("sockname"));
    }
  if (debug) cout<<"try open ved"<<endl;
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(-1);
  }
try
  {
  ved=new C_VED(vedname);
  is=ved->is0();
  proclist=is->newproclist();
  proclist->add_proc("Timestamp");
  proclist->add_par(1);
  proclist->add_proc("GetSyncStatist");
  proclist->add_par(4, hists, 0, maxchan, 0);
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  communication.done();
  return(-1);
  }
if (verbose)
  cout<<"ved \""<<ved->name()<<"\" open via "<<communication<<endl;
return(0);
}

/*****************************************************************************/

void close_ved()
{
try
  {
  delete proclist;
  proclist=0;
  delete ved;
  communication.done();
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
}

/*****************************************************************************/

int Statist::MakeHist(int nbin, float x0, float x1)
{
int need_h=1, need_ih=0, need_ith=0;
switch (typ)
  {
  case 1:
    need_ih=1; need_ith=1;
    name="ldt"; iname="ildt"; itname="itldt"; title="Local Dead Time";
    break;
  case 2:
    need_ih=1; need_ith=1;
    name="tdt"; iname="itdt"; itname="ittdt"; title="Total Dead Time";
    break;
  case 4:
    //need_ih=1;
    name="gap"; iname="igap"; itname="itgap"; title="Trigger Distribution";
    break;
  default:
    name="unknown"; iname="iunknown"; itname="itunknown"; title="Unknown";
    break;
  }
pad->histpad->cd();
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

int Statist::Read(C_inbuf& ib, int scale)
{
ib>>typ>>entries>>ovl>>min>>max>>sum>>qsum>>histsize>>histscale;
newscale=scale;
ib>>arrdim.nbin>>leer;

bin_width=0.1; // 100 ns in us
if (histscale) bin_width*=histscale;
if (newscale) bin_width*=newscale;
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
if (pad)
  {
  char ttt[500];
  if (typ==4)
    {
    if (entries)
      sprintf(ttt, "%d entries  %f%% overflow\n",
          entries, (float)ovl/entries*100.);
    else
       sprintf(ttt, "%d entries\n", entries);
    }
  else
    {
    if (entries)
      sprintf(ttt, "%d entries  %f%% overflow  mean=%f us\n",
          entries,
          (float)ovl/entries*100.,
          (float)sum/(float)entries*0.1);
    else
      sprintf(ttt, "%d entries\n", entries);
    }
  pad->text->SetLabel(ttt);
  pad->pad->Modified();
  pad->histpad->Modified();
  pad->text->Modified();
  }
return 0;
}

/*****************************************************************************/

void Statist::print(ostream& o)
{
o<<"typ="<<typ<<"; entries="<<entries<<"; ovl="<<ovl<<endl;
o<<"max="<<max<<"; min="<<min<<"; sum="<<sum<<"; qsum="<<qsum<<endl;
o<<"histsize="<<histsize<<"; histscale="<<histscale<<endl;
o<<"newscale="<<newscale<<"; newsize="<<arrdim.nbin<<"; leer="<<leer<<endl;
}

/*****************************************************************************/
int getstatist()
{
try
  {
  struct timeval tv;
  C_confirmation* conf=is->command(proclist);
  //C_confirmation* conf=is->command("GetSyncStatist", 4, 7 ,0 ,maxchan ,0);
  C_inbuf ib(conf->buffer(), conf->size());
  ib++; // skip errorcode
  if (debug) cout<<ib<<endl;
  ib>>tv.tv_sec>>tv.tv_usec;
  ib++; // skip counter
  int evnum;
  unsigned long rejected;
  ib >> evnum >> rejected;
  ostrstream ss;
  ss.setf(ios::scientific, ios::floatfield);
  //ss.precision(4);
  ss<<"trigger: "<<evnum+rejected<<"; rejected: "<<rejected;
  if (rejected)
    {
    double percent=(double)rejected/(double)(evnum+rejected)*100.;
    ss.setf(0, ios::floatfield);
    ss.precision(3);
    ss<<" ("<<percent<<" %)";
    }
  ss<<ends;
XXX das geht nicht bei ANSI C++
  char* cs=ss.str();
  Events->Clear();
  TText* t=Events->AddText(cs);
  t->SetTextFont(20);
  delete[] cs;
  if (last_tv.tv_sec && (evnum>=last_evnum) && (rejected>=last_rejected))
    {
    ostrstream ss;
    double erate, trate;
    double tdiff=tv.tv_sec-last_tv.tv_sec+
        (double)(tv.tv_usec-last_tv.tv_usec)/1000000.;
    erate=(double)(evnum-last_evnum)/tdiff;
    trate=(double)(rejected-last_rejected)/tdiff+erate;
    ss<<"trigger rate: "<<trate<<"/s; measured rate: "<<erate<<"/s"<<ends;
    char* cs=ss.str();
    TText* t=Events->AddText(cs);
    t->SetTextFont(20);
    delete[] cs;
    }
  last_tv=tv; last_evnum=evnum; last_rejected=rejected;
  int num_arrs=ib.getint();
  if (verbose) cout<<"evnum="<<evnum<<"; rejected="<<rejected<<endl;
  if (verbose) cout<<"num_arrs="<<num_arrs<<endl;
  int i;
  for (i=0; (i<num_arrs) && (i<nhist); i++)
    {
    if (debug) cout<<ib<<endl;
    pads[i].pad->cd(i);
    statist[i].Read(ib, 0);
    if (verbose) statist[i].print(cout);
    }
  if (debug) cout<<ib<<endl;
  return num_arrs;
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return -1;
  }
return 0;
}
/*****************************************************************************/
void periodic_handler()
{
int num_arrs;
if (!ved_is_open)
  {
  if (open_ved()<0) return;
  ved_is_open=1;
  }
if ((num_arrs=getstatist())<=0)
  {
  close_ved();
  ved_is_open=0;
  return;
  }
if (debug) cout<<"getstatist ok."<<endl;
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

int make_canvas()
{
gStyle->SetCanvasColor(10);
gStyle->SetPadColor(10);
char wtitle[100], title[100], res_name[100];
sprintf(wtitle, "Readout Statistics (%s)", vedname);
sprintf(title, "Readout Statistics for VED %s", vedname);
sprintf(res_name, "syncdisplay_%s", vedname);
const int titlesize=35; // Pixel
const int eventsize=50; // Pixel
const int histsize=250; // Pixel
const int textsize=30;  // Pixel
int canvsize=titlesize+eventsize+(histsize+textsize)*nhist;
float titlediv=(float)titlesize/(float)canvsize;
float eventdiv=(float)eventsize/(float)canvsize;
if ((canvas=new TCanvas(res_name, wtitle, 640, canvsize))==0)
    return -1;
canvas->SetEditable(kIsNotEditable);
canvas->SetFillColor(42);
canvas->GetFrame()->SetFillColor(21);
canvas->GetFrame()->SetBorderSize(6);
canvas->GetFrame()->SetBorderMode(-1);
const float sp=0.003;
//const float sp=0.;
TPaveLabel* Title = new TPaveLabel(sp,1.-titlediv+sp,1.-sp,1.-sp, title);
Title->SetTextFont(20);
Title->Draw();
Events=new TPaveText(sp,1.-titlediv-eventdiv+sp,1.,1.-titlediv-sp, "AAA");
Events->Draw();
histpad=new TPad("histpad", "HistPad", 0., 0., 1., 1.-titlediv-eventdiv,
    42, 5, 0);
histpad->Draw();
for (int i=0; i<nhist; i++)
  {
  histpad->cd();
  char pname[100], hpname[100];
  sprintf(pname, "%s_%d", "histpad", i+1);
  sprintf(hpname, "%s_%d", "hhistpad", i+1);
  //const float sp=0.002;
  const float sp=0.;
  const float textsize=.07;
  float x0, x1, y0, y1;
  x0=sp; x1=1.-sp;
  y0=(nhist-i-1)*(1./nhist)+sp;
  y1=(nhist-i)*(1./nhist)-sp;
  pads[i].pad=new TPad(pname, pname, x0, y0, x1, y1, 17, 0, 0);
  pads[i].pad->Draw();
  pads[i].pad->cd();
  pads[i].histpad=new TPad(hpname, hpname, 0., textsize, 1., 1., 17, 5, 1);
  pads[i].histpad->Draw();
  pads[i].text=new TPaveLabel(0.,0.,1.,textsize, "AAA");
  pads[i].text->SetTextFont(20);
  pads[i].text->Draw();
  }

//gObjectTable->Print();
}

/*****************************************************************************/

main(int argc, char* argv[])
{
try
  {
  TApplication app("SyncDisplay", /*&argc*/0, /*argv*/0);
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  statist=new Statist[nhist];
  pads=new Tpads[nhist];
  ved_is_open=0;
  proclist=0;
  last_tv.tv_sec=0;
  if (make_canvas()<0)
    {
    cout<<"root problem"<<endl;
    return 1;
    }
  for (int i=0; i<nhist; i++) statist[i].pad=pads+i;
  periodic_handler();
  TUpdateTimer* timer=new TUpdateTimer(args->getintopt("interval"));
  gSystem->AddTimer(timer);
  signal(SIGFPE, SIG_DFL);
  app.Run(kTRUE);
  cout<<"bin zurueck aus app->Run"<<endl;
  delete timer;
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
delete ved;
communication.done();
}

/*****************************************************************************/
/*****************************************************************************/
