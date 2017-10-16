/******************************************************************************
*                                                                             *
* recscan.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 22.09.95                                                           *
* last changed: 22.09.95                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <events.hxx>
#include <inbuf.hxx>
#include <compat.h>
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: headscan.cc,v 1.6 2005/02/11 15:45:08 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose;
int debug;
int fast;
int buffered;
STRING infile;
STRING suffix;
STRING prefix;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "verbose", false, "verbose");
args->addoption("debug", "debug", false, "debug");
args->addoption("no_tape", "nt", false, "input not from tape");
//args->addoption("fast", "fast", false, "fast");
args->addoption("infile", 0, "/dev/nrmt0h", "input file", "input");
//args->addoption("usrprefix", "usrprefix", "",
//    "prefix to be used for userrecords", "prefix");
//args->addoption("usrsuffix", "usrsuffix", "",
//    "suffix to be used for userrecords", "suffix");
//args->hints(C_readargs::exclusive, "fast", "no_tape", 0);

if (args->interpret(1)!=0) return(-1);

verbose=args->getboolopt("verbose");
debug=args->getboolopt("debug");
//fast=args->getboolopt("fast");
buffered=args->getboolopt("no_tape");
infile=args->getstringopt("infile");
//suffix=args->getstringopt("usrsuffix");
//prefix=args->getstringopt("usrprefix");

return(0);
}

/*****************************************************************************/

void printwatzstring(C_inbuf& in, int size, ofstream& f)
{
int* r=new int[size+1];
int i;
for (i=0; i<size; i++) r[i]=in.getint();
r[size]=0;
f << (char*)r;
delete[] r;
}

/*****************************************************************************/

void printwatzstring(C_inbuf& in, ofstream& f)
{
printwatzstring(in, in.getint(), f);
}

/*****************************************************************************/

void printwatzfile(C_inbuf& in, ofstream& f)
{
int recs=in.getint();
int i;
for (i=0; i<recs; i++)
  {
  printwatzstring(in, f);
  f << endl;
  }
}

/*****************************************************************************/

class C_header
  {
  public:
    C_header();
    virtual ~C_header();
  protected:
    int path;
    ofstream* f;
    STRING fehler;
    int nr_of_recs;
    int rec_no;
    int need_more;
    virtual int type() const =0;
    virtual void dump_record(C_inbuf&)=0;
    int open_file(const char*);
  public:
    virtual int accept(C_inbuf&);  // -1: not accepted
                                   //  0: ok; ready
                                   //  1: ok, need more records
    STRING error() const {return fehler;}
  };

class C_main_header: public C_header
  {
  public:
    C_main_header();
    virtual ~C_main_header();
    virtual int type() const {return 1;}
    virtual void dump_record(C_inbuf& ib);
  };

class C_conf_header: public C_header
  {
  public:
    C_conf_header();
    virtual ~C_conf_header();
    virtual int type() const {return 2;}
    virtual void dump_record(C_inbuf& ib);
  };

class C_modul_header: public C_header
  {
  public:
    C_modul_header();
    virtual ~C_modul_header();
    virtual int type() const {return 3;}
    virtual void dump_record(C_inbuf& ib);
  };

class C_sync_header: public C_header
  {
  public:
    C_sync_header();
    virtual ~C_sync_header();
    virtual int type() const {return 4;}
    virtual void dump_record(C_inbuf& ib);
  };

class C_is_header: public C_header
  {
  public:
    C_is_header();
    virtual ~C_is_header();
    virtual int type() const {return 5;}
    virtual void dump_record(C_inbuf& ib);
  };

class C_user_header: public C_header
  {
  public:
    C_user_header();
    virtual ~C_user_header();
    virtual int type() const {return 6;}
    virtual void dump_record(C_inbuf& ib);
  };

class C_dummy_header: public C_header
  {
  public:
    C_dummy_header();
    virtual ~C_dummy_header();
    virtual int type() const {return 0;}
    virtual void dump_record(C_inbuf& ib) {}
    virtual int accept(C_inbuf&) {fehler="dummy"; return -1;}
  };

/*****************************************************************************/

C_header::C_header()
:path(-1), f(0), nr_of_recs(0), rec_no(0)
{}

C_header::~C_header()
{
if (rec_no!=nr_of_recs)
    (*f) << nr_of_recs-rec_no << "records missing" << endl;
delete f;
if (path>=0) close(path);
}

C_main_header::C_main_header(void)
{}
C_conf_header::C_conf_header(void)
{}
C_modul_header::C_modul_header(void)
{}
C_sync_header::C_sync_header(void)
{}
C_is_header::C_is_header(void)
{}
C_user_header::C_user_header(void)
{}
C_dummy_header::C_dummy_header(void)
{}

C_main_header::~C_main_header(void)
{}
C_conf_header::~C_conf_header(void)
{}
C_modul_header::~C_modul_header(void)
{}
C_sync_header::~C_sync_header(void)
{}
C_is_header::~C_is_header(void)
{}
C_user_header::~C_user_header(void)
{}
C_dummy_header::~C_dummy_header(void)
{}

/*****************************************************************************/

int C_header::open_file(const char* name)
{
int idx=0;
do
  {
  OSSTREAM ss;
  ss << name << '_' << idx << ends;
  STRING s=ss.str();
  path=open(s.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
  if ((path<0) && (errno!=EEXIST))
    {
    clog << "open " << s << ": " << strerror(errno) << endl;
    return -1;
    }
  idx++;
  }
while (path<0);
f=new ofstream(path);
return 0;
}

/*****************************************************************************/

void C_main_header::dump_record(C_inbuf& ib)
{
if (rec_no!=1)
  {
  clog << "main header: rec_no!=1 nicht zulaessig" << endl;
  return;
  }

if (!f)
  {
  open_file("main");
  if (!f) return;
  }

ib+=6; // ... ueberspringen
(*f) << "run " << ib.getint() << endl;
printwatzstring(ib, (28+3)/4, *f); (*f) << endl;
printwatzstring(ib, (80+3)/4, *f); (*f) << endl;
printwatzstring(ib, (80+3)/4, *f); (*f) << endl;
}

/*****************************************************************************/

void C_conf_header::dump_record(C_inbuf& ib)
{
if (!f)
  {
  open_file("hwconf");
  if (!f) return;
  }
ib+=7;
printwatzfile(ib, *f);
}

/*****************************************************************************/

void C_modul_header::dump_record(C_inbuf& ib)
{
if (!f)
  {
  open_file("modul");
  if (!f) return;
  }
ib+=7;
printwatzfile(ib, *f);
}

/*****************************************************************************/

void C_sync_header::dump_record(C_inbuf& ib)
{
if (!f)
  {
  open_file("sync");
  if (!f) return;
  }
ib+=7;
printwatzfile(ib, *f);
}

/*****************************************************************************/

void C_is_header::dump_record(C_inbuf& ib)
{
const char* isnames[]=
  {
  "fera",
  "fastbus multi block",
  "struck dl350",
  "lecroy tdc2277",
  "fastbus lecroy",
  "camac scaler",
  "zel discriminator",
  "fastbus kinetic",
  "drams",
  "lecroy tdc2228",
  "multi purpose",
  };

ib+=5;
int is=ib.getint()-1;
ib++; // run_nr ueberspringen
const char* name;
STRING st;

if (!f)
  {
  if ((unsigned int)is>10)
    {
    OSSTREAM ss;
    ss << "unknown is nr. " << is+1 << ends;
    st=ss.str();
    name=st.c_str();
    }
  else
    name=(char*)isnames[is];

  open_file(name);
  if (!f) return;
  (*f) << "! IS type: " << name << endl;
  (*f) << "! =============================" << endl;
  }
printwatzfile(ib, *f);
}

/*****************************************************************************/

void C_user_header::dump_record(C_inbuf& ib)
{
if (!f)
  {
  open_file("user");
  if (!f) return;
  }
ib+=7;
printwatzfile(ib, *f);
}

/*****************************************************************************/

int C_header::accept(C_inbuf& ib)
{
if (ib[3]!=type())
  {
  OSSTREAM ss;
  ss << "Falscher Record-Typ; got " << ib[3] << " but " << type()
      << " expected" << ends;
  fehler=ss.str();
  return -1;
  }
int subtype=ib[4];
int nrecs=(subtype>>16)&0xff;
int recno=subtype&0xff;
if (rec_no+1!=recno)
  {
  OSSTREAM ss;
  ss << "Falsche Record-Nummer; got " << recno << " but " << rec_no+1
      << " expected" << ends;
  fehler=ss.str();
  return -1;
  }
if (nr_of_recs && (nr_of_recs!=nrecs))
  {
  OSSTREAM ss;
  ss << "Falsche Record-Anzahl; got " << nrecs << " but " << nr_of_recs
      << " expected" << ends;
  fehler=ss.str();
  return -1;
  }
// record accepted;
nr_of_recs=nrecs;
rec_no=recno;
need_more=(nrecs>recno);
dump_record(ib);
return need_more;
}

/*****************************************************************************/

C_header* kopf=0;

void kopf_ab()
{delete kopf; kopf=0;}

/*****************************************************************************/

void do_header(C_eventp& event)
{
C_inbuf in(event.buffer());
int res;

if (kopf && ((res=kopf->accept(in))==-1)) kopf_ab();

if (!kopf)
  {
  switch (in[3])  // record_type
    {
    case 1: kopf=new C_main_header; break;
    case 2: kopf=new C_conf_header; break;
    case 3: kopf=new C_modul_header; break;
    case 4: kopf=new C_sync_header; break;
    case 5: kopf=new C_is_header; break;
    case 6: kopf=new C_user_header; break;
    default: kopf=new C_dummy_header; break;
    }
  if ((res=kopf->accept(in))==-1)
    {
    clog << kopf->error() << endl;
    clog << "irgendwas faul" << endl;
    kopf_ab();
    }
  }
if (res==0) kopf_ab();
}

/*****************************************************************************/

void scan_slow(C_evinput* evin)
{
C_eventp event;
int fno=1;

try
  {
  while (!evin->fatal())
    {
    int eventrecs=0;
    int headrecs=0;
    OSSTREAM ss;
    ss << "file_" << fno << ends;
    STRING st=ss.str();
    const char* s=st.c_str();
    if (mkdir(s, 0755)==-1)
        clog << "mkdir " << s << ": " << strerror(errno) << endl;
    if (chdir(s)==-1)
        clog << "chdir " << s << ": " << strerror(errno) << endl;
    while ((*evin) >> event)
      {
      if (event.evnr()==0)
        {
        if (eventrecs)
          {
          if (verbose) clog << eventrecs << " eventrecords" << endl;
          eventrecs=0;
          }
        headrecs++;
        do_header(event);
        }
      else
        {
        if (headrecs)
          {
          if (verbose) clog << headrecs << " headerrecords" << endl;
          headrecs=0;
          }
        eventrecs++;
        }
      }
    cerr << "filemark" << endl;
    evin->reset(); // fehler loeschen
    kopf_ab();
    if (chdir("..")==-1)
        clog << "chdir ..: " << strerror(errno) << endl;
    fno++;
    }
  cerr << "fatal error" << endl;
  }
catch (C_error* error)
  {
  cout << (*error) << endl;
  delete error;
  }
}

/*****************************************************************************/

void scan_file()
{
C_evinput* evin;

try
  {
  if (debug) clog << "try to open" << endl;
  if (buffered)
    {
    if (debug) clog << "open buffered" << endl;
    evin=new C_evfinput(infile, 65536/4);
    }
  else
    {
    if (debug) clog << "open tape" << endl;
    evin=new C_evtinput(infile, 65536/4);
    }
  if (debug) clog << "open ok" << endl;
  }
catch (C_error* err)
  {
  cerr << (*err) << endl;
  delete err;
  return;
  }

scan_slow(evin);

delete evin;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

scan_file();

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
