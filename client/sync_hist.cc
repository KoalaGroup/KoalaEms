/*
 * sync_hist.cc
 * 
 * created: 02.06.98
 * last changed: 02.06.98
 */

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
#include <histogram.hxx>
#include "client_defaults.hxx"

#include <versions.hxx>

VERSION("Mar 27 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: sync_hist.cc,v 1.4 2004/11/26 22:20:50 wuestner Exp $")

C_VED* ved;
C_instr_system* is;
C_proclist* proclist;
C_readargs* args;
int verbose, debug;
int hists, nhist, maxchan;
const char* vedname;
int last_evnum;
unsigned long last_rejected;
String filename;

/*****************************************************************************/

int readargs()
{
int res;
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("file", "f", "",
    "file(stub) for histogram(s)", "filename");
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK,
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("hists", "hists", 7, "histogram mask", "hists");
args->addoption("maxchan", "maxchan", 0, "number of channels", "maxchan");
args->addoption("vedname", 0, "", "Name des VED", "vedname");
args->hints(C_readargs::required, "vedname", 0);
args->hints(C_readargs::required, "file", 0);
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
filename=args->getstringopt("file");
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
histogram<int>* Read(C_inbuf& ib)
{
int typ, entries, ovl, min, max, nbin, size, scale, leer, num;
float xmin, xmax;
unsigned long sum, qsum;

ib>>typ>>entries>>ovl>>min>>max>>sum>>qsum>>size>>scale;
ib>>num;
if (num)
  {
  ib>>leer;
  nbin=num-leer;
  if (nbin==0)
    {
    cerr<<"histogramm ist leer"<<endl;
    return 0;
    }
  if (verbose)
    {
    cerr<<"typ="<<typ<<"; entries="<<entries<<"; ovl="<<ovl<<endl;
    cerr<<"max="<<max<<"; min="<<min<<"; sum="<<sum<<"; qsum="<<qsum<<endl;
    cerr<<"size="<<size<<"; scale="<<scale<<endl;
    cerr<<"nbin="<<nbin<<"; leer="<<leer<<endl;
    }
  if (scale==0) scale=1;
  xmin=leer*0.1/*ms*/*scale;
  xmax=num*0.1/*ms*/*scale;
  if (verbose)
    {
    cerr<<"xmax="<<xmax<<"; xmin="<<xmin<<endl;
    }
  char* hname;
  switch (typ)
    {
    case 1: hname="ldt"; break;
    case 2: hname="tdt"; break;
    case 4: hname="tdist"; break;
    default: hname="unknown"; break;
    }
  if ((xmin>=xmax) || (nbin==0))
    {
    cerr<<"fatal error in Read: xmax="<<xmax<<"; xmin="<<xmin<<"; nbin="<<nbin<<endl;
    ib+=nbin;
    return 0;
    }
  histogram<int>* hi=new histogram<int>(hname, 1, xmin, xmax, nbin);
  for (int i=0; i<nbin; i++)
    {
    int v=ib.getint();
    hi->set(i, v);
    }
  return hi;
  }
else
  return 0;
}
/*****************************************************************************/
int getstatist()
{
try
  {
  struct timeval tv;
  C_confirmation* conf=is->command(proclist);
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
  cerr<<cs<<endl;
  delete[] cs;
  int num_arrs=ib.getint();
  if (verbose) cerr<<"evnum="<<evnum<<"; rejected="<<rejected<<endl;
  if (verbose) cerr<<"num_arrs="<<num_arrs<<endl;
  int i;
  for (i=0; (i<num_arrs) && (i<nhist); i++)
    {
    if (debug) cerr<<ib<<endl;
    histogram<int>* h=Read(ib);
    if (h)
      {
      String fname;
      fname=filename+"_"+h->name();
      C_iopath path(fname, C_iopath::iodir_output);
      if (path.typ()==C_iopath::iotype_none)
        {
        cerr<<"error open path for "<<fname<<" "<<endl;
        }
      else
        {
        C_outbuf ob;
        ob<<(*h);
        if (ob.write(path)<0) cerr<<"write error"<<endl;
        }
      delete h;
      }
    }
  if (debug) cerr<<ib<<endl;
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
main(int argc, char* argv[])
{
try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(1);

  proclist=0;
  if (open_ved()<0) return 2;
  getstatist();
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
/*****************************************************************************/
