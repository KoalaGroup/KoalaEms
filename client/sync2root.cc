/******************************************************************************
*                                                                             *
* sync2root.cc                                                                *
*                                                                             *
* created: 18.10.97                                                           *
* last changed: 18.10.97                                                      *
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

#include <versions.hxx>

VERSION("Oct 10 1997", __FILE__, __DATE__, __TIME__,
"$ZEL: sync2root.cc,v 1.4 2004/11/26 22:20:49 wuestner Exp $")

class Statist
  {
  public:
    Statist() :hist(0) {}
    ~Statist() {delete hist;}
    const char* name;
    const char* title;
    void read(C_inbuf&, int);
    void print(ostream&);
    void Write(int);
    float bin_width;
    float upper_limit;
    int typ;
    int entries, ovl;
    int max, min;
    unsigned long sum, qsum;
    int histsize, histscale;
    int newscale, newsize, leer;
    int* hist;
  };

TROOT GlobalRoot("Wurzel", "This is to make ROOT happy");

C_VED* ved;
C_readargs* args;
int verbose;
int debug;
const char* vedname;
String rootfilename;
int num_arrs;

Statist statist[3];

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
args->addoption("vedname", 0, "", "Name des VED", "vedname");
args->addoption("rootfile", 1, "", "name of rootfile, default: sync_<ved>.root",
     "rootfilename");
args->hints(C_readargs::required, "vedname", 0);
args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
args->hints(C_readargs::exclusive, "port", "sockname", 0);

if ((res=args->interpret(1))!=0) return res;

debug=args->getboolopt("debug");
verbose=debug || args->getboolopt("verbose");

vedname=args->getstringopt("vedname");
if (args->isdefault("rootfile"))
  {
  rootfilename="sync_";
  rootfilename+=vedname;
  rootfilename+=".root";
  }
else
  rootfilename=args->getstringopt("rootfile");
if (debug)
  {
  cout<<"vedname =\""<<vedname<<"\""<<endl;
  cout<<"rootname=\""<<rootfilename<<"\""<<endl;
  }
return 0;
}

/*****************************************************************************/

int open_ved()
{
try
  {
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
  ved=new C_VED(vedname);
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(-1);
  }
if (verbose)
  cout<<"ved \""<<ved->name()<<"\" open via "<<communication<<endl;
return(0);
}

/*****************************************************************************/

void Statist::read(C_inbuf& ib, int scale)
{
delete hist; hist=0;
ib>>typ>>entries>>ovl>>max>>min>>sum>>qsum>>histsize>>histscale;
newscale=scale;
ib>>newsize>>leer;
hist=new int[newsize];
if (hist==0) throw new C_unix_error(errno, "alloc array");
int i;
for (i=0; i<leer; i++) hist[i]=0;
for (; i<newsize; i++) hist[i]=ib.getint();
bin_width=0.1; // 100 ns
if (histscale) bin_width*=histscale;
if (newscale) bin_width*=newscale;
upper_limit=bin_width*newsize;
if (name==0)
  {
  switch (typ)
    {
    case 1: name="ldt"; break;
    case 2: name="tdt"; break;
    case 4: name="gap"; break;
    default: name="unknown"; break;
    }
  }
if (title==0)
  {
  switch (typ)
    {
    case 1: title="Local Dead Time"; break;
    case 2: title="Total Dead Time"; break;
    case 4: title="Trigger Gaps"; break;
    default: title="Unknown"; break;
    }
  }
}

/*****************************************************************************/

void Statist::print(ostream& o)
{
o<<"typ="<<typ<<"; entries="<<entries<<"; ovl="<<ovl<<endl;
o<<"max="<<max<<"; min="<<min<<"; sum="<<sum<<"; qsum="<<qsum<<endl;
o<<"histsize="<<histsize<<"; histscale="<<histscale<<endl;
o<<"newscale="<<newscale<<"; newsize="<<newsize<<"; leer="<<leer<<endl;
if (debug) for (int i=leer; (i<newsize) && (i<leer+10); i++)
  o<<"  ["<<i<<"] "<<hist[i]<<endl;
}

/*****************************************************************************/
void Statist::Write(int k)
{
TH1F* h=new TH1F(name, title, newsize, 0., upper_limit);
if (h==0) {cerr<<"new TH1F sclug fehl"<<endl; return;}
h->SetXTitle("`m#s");
for (int i=0; i<newsize; i++) h->AddBinContent(i, hist[i]);
if (debug) cout<<"delete TH1F("<<name<<")"<<endl;
h->Write();
delete h;
}
/*****************************************************************************/

void wurzele()
{
TFile* hfile=new TFile("sync_024.root","RECREATE","Synchronisation Statistics");
for (int i=0; i<num_arrs; i++)
  {
  statist[i].Write(i);
  }
hfile->Write();
hfile->Close();
delete hfile;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  debug=args->getboolopt("debug");
  verbose=debug || args->getboolopt("verbose");

  if (open_ved()==0)
    {
    C_instr_system* is=ved->is0();
    C_confirmation* conf=is->command("GetSyncStatist", 4, 7 ,0 ,30000 ,0);
    C_inbuf ib(conf->buffer(), conf->size());
    ib+=2; // skip errorcode and counter
    int evnum;
    unsigned long rejected;
    ib >> evnum >> rejected;
    if (verbose) cout<<"evnum="<<evnum<<"; rejected="<<rejected<<endl;
    num_arrs=ib.getint();
    int i;
    for (i=0; i<num_arrs; i++) statist[i].read(ib, 0);
    if (verbose) for (i=0; i<num_arrs; i++) statist[i].print(cout);
    //if (debug) cout<<ib<<endl;
    wurzele();
    }
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
