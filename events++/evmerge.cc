/******************************************************************************
*                                                                             *
* evmerge.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 13.12.95                                                           *
* last changed: 13.12.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <sys/ioctl.h>
#include <events.hxx>
#include <compat.h>
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: evmerge.cc,v 1.6 2005/02/11 15:45:06 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int debug;
int tapeoutput;
int usr_save;
int del_isnum;
int* del_is;
int no_del;
int isoffs;
int maxevnum;
int maxevid;
STRING outfile;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("tape_out", "to", false, "output is tape");
args->addoption("nobuffered_out", "nb", false, "output is not buffered");
args->addoption("swap", "swap", false, "swap words in output");
args->addoption("maxevnum", "evmax", -1, "maximum number of events to be stored",
    "num");
args->addoption("maxevid", "idmax", -1, "id of last event to be stored", "id");
args->addoption("maxexpect", "maxexp", 0, "maximum expected eventsize", "size");
args->addoption("usr_save", "usr_save", false,
    "don't delete userrecords from eventstream");
args->addoption("delay_is", "delay_is", -1, "IS to be delayed", "ISid");
args->multiplicity("delay_is", 0);
args->addoption("no_delay_is", "no_delay_is", -1, "IS not to be delayed", "ISid");
args->multiplicity("no_delay_is", 0);
args->hints(C_readargs::exclusive, "delay_is", "no_delay_is", 0);
args->addoption("isoffs", "offset", 0, "Offset to be applied to delayed IS",
    "offs");
args->addoption("infile", "infile", "-", "input file (- - for stdin)", "infile");
args->multiplicity("infile", 0);
args->addoption("intape", "intape", "/dev/nrmt0h", "input tape", "device");
args->multiplicity("intape", 0);
args->addoption("outfile", 0, "-", "output file", "output");
if (args->interpret(1)!=0) return -1;

int i;

verbose=args->getboolopt("verbose");
debug=args->getboolopt("debug");
tapeoutput=args->getboolopt("tape_out");
outfile=args->getstringopt("outfile");

no_del=(args->multiplicity("no_delay_is")>0);
del_isnum=args->multiplicity("delay_is")+args->multiplicity("no_delay_is");
del_is=new int[del_isnum];
if (del_isnum>0)
  {
  if (no_del)
    for (i=0; i<del_isnum; i++) del_is[i]=args->getintopt("no_delay_is", i);
  else
    for (i=0; i<del_isnum; i++) del_is[i]=args->getintopt("delay_is", i);
  }
isoffs=args->getintopt("isoffs");
if (verbose)
  {
  if (del_isnum)
    {
    clog << del_isnum << " IS werden " << (no_del?"nicht ":"")
        << " verschoben:";
    for (i=0; i<del_isnum; i++) clog << ' ' << del_is[i];
    clog << endl;
    clog << "offset ist " << isoffs << endl;
    }
  else
    {
    clog << "es werden keine SubEvents verschoben" << endl;
    }
  }

usr_save=args->getboolopt("usr_save");
maxevnum=args->getintopt("maxevnum");
maxevid=args->getintopt("maxevid");

return(0);
}

/*****************************************************************************/

int do_merge()
{
C_evinput** evin;
int numin;
C_evoutput* evout;
C_eventp* inevent;
C_event outevent;
C_event delayed_event;
C_subeventp subevent;

if (verbose)
  {
  clog << args->multiplicity("intape") << " tapes for input" << endl;
  clog << args->multiplicity("infile") << " files for input" << endl;
  }
numin=args->multiplicity("infile")+args->multiplicity("intape");

if ((evin=new C_evinput*[numin])==0)
  {
  clog << "allocate memory: " << strerror(errno);
  return -1;
  }
if ((inevent=new C_eventp[numin])==0)
  {
  clog << "allocate memory: " << strerror(errno);
  return -1;
  }

try
  {
  int n=0, nn;
  for (nn=0; nn<args->multiplicity("intape"); nn++, n++)
    {
    if (verbose) clog << "open tape " << args->getstringopt("intape", nn) << endl;
    evin[n]=new C_evtinput(args->getstringopt("intape", nn), 65536);
    evin[n]->setmaxexpect(args->getintopt("maxexpect"));
    }
  for (nn=0; nn<args->multiplicity("infile"); nn++, n++)
    {
    if (verbose) clog << "open file " << args->getstringopt("infile", nn) << endl;
    evin[n]=new C_evfinput(args->getstringopt("infile", nn), 65536);
    evin[n]->setmaxexpect(args->getintopt("maxexpect"));
    }
  if (verbose) clog << "open outfile " << outfile << endl;
  if (tapeoutput)
    evout=new C_evtoutput(outfile, 65536);
  else if (!args->getboolopt("nobuffered_out"))
    evout=new C_evfoutput(outfile, 65536);
  else
    evout=new C_evpoutput(outfile, 65536);
  evout->swap(args->getboolopt("swap"));
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return(-1);
  }

try
  {
  int saved=0;
  int weiter=1;
  int lastevn=0;
  for (int n=0; n<numin; n++)
    {
    if (debug) clog << "read from " << evin[n]->name() << endl;
    *(evin[n]) >> inevent[n];
    }
  delayed_event.clear();
  try
    {
    do // fuer jedes event
      {
      int n, evn;
      // testen, ob noch alle Quellen Daten haben
      for (n=0; n<numin; n++)
        {
        if (!inevent[n])
          {
          clog << evin[n]->name() << ": daten alle" << endl;
          throw 0;
          }
        }
      // kleinste Eventnummer suchen
      evn=inevent[0].evnr();
      for (n=1; n<numin; n++) if (inevent[n].evnr()<evn) evn=inevent[n].evnr();
      if (debug) clog << "evn=" << evn << endl;
      if (evn!=lastevn+1)
        {
        clog << "event: " << lastevn << " ==> " << evn << endl;
        }
      lastevn=evn;
      if ((evn==0) && !usr_save)
        {
        for (n=0; n<numin; n++)
          if (inevent[n].evnr()==evn)
            {
            inevent[n].clear();
            if (debug) clog << "usr_rec cleared; read from " << evin[n]->name() << endl;
            *(evin[n]) >> inevent[n];
            }
        }
      else
        {
        int tnr, has_tnr=0;
        outevent.clear();
        outevent.evnr(evn);
        if (delayed_event.valid())
          {
          if (delayed_event.evnr()!=evn)
            clog << "delayed_event.evnr()="<< delayed_event.evnr() << "; "
                << evn << " expected" << endl;
          while (delayed_event >> subevent) outevent << subevent;
          }
        delayed_event.clear();
        for (n=0; n<numin; n++)
          if (inevent[n].evnr()==evn)
            {
            if (has_tnr && (inevent[n].trignr()!=tnr))
              {
              clog << evin[n]->name() << "; event " << evn << " has trigger "
                  << inevent[n].trignr() << " instead of " << tnr << endl;
              }
            else
              {
              tnr=inevent[n].trignr();
              outevent.trignr(tnr);
              }
            while (inevent[n] >> subevent)
              {
              int sevn=subevent.id();
              int del=0;
              if (del_isnum)
                {
                del=no_del;
                for (int i=0; i<del_isnum; i++) if (sevn==del_is[i]) del=!no_del;
                }
              if (del)
                {
                delayed_event.evnr(evn+1);
                if (isoffs) subevent.id(subevent.id()+isoffs);
                delayed_event << subevent;
                }
              else
                outevent << subevent;
              }
            if (debug) clog << "read from " << evin[n]->name() << endl;
            *(evin[n]) >> inevent[n];
            }
          else
            {
            clog << evin[n]->name() << "has no event " << evn << endl;
            }
        (*evout) << outevent;
        if ((++saved>=maxevnum) && (maxevnum>0)) weiter=0;
        if ((maxevid>0) && (evn>=maxevid)) weiter=0;
        }
      }
      while (weiter);
      }
    catch (int)
      {}
    clog << saved << " events stored" << "; last event: " << lastevn << endl;
  }
catch (C_error* error)
  {
  clog << (*error) << endl;
  delete error;
  }

for (int n=0; n<numin; n++) delete evin[n];
delete evout;
return 0;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

if (do_merge()!=0)
  return 1;
else
  return 0;
}

/*****************************************************************************/
/*****************************************************************************/
