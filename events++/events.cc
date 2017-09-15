/*
 * events++/events.cc
 * 
 * created 27.01.95 PW
 */

#include <errors.hxx>
#include <events.hxx>
#include <cerrno>
#include <cstring>
#include "versions.hxx"

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: events.cc,v 1.7 2010/09/04 21:18:49 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_event_manipi::C_event_manipi(event_manipi f, int arg)
:f(f), arg(arg)
{}

/*****************************************************************************/

C_eventp& operator >>(C_eventp& event, const C_event_manipi& m)
{
return(m.f(event, m.arg));
}

/*****************************************************************************/

C_eventp& operator <<(C_eventp& event, const C_event_manipi& m)
{
return(m.f(event, m.arg));
}

/*****************************************************************************/

C_eventp& operator >>(C_eventp& event, const C_event_manipir& m)
{
return(m.f(event, m.arg));
}

/*****************************************************************************/

C_eventp& operator <<(C_eventp& event, const C_event_manipir& m)
{
return(m.f(event, m.arg));
}

/*****************************************************************************/

C_eventp& setcopy_(C_eventp& event, int x)
{
event.setcopy(x);
return(event);
}

C_event_manipi setcopy(int x)
{
return C_event_manipi(&setcopy_, x);
}

/*****************************************************************************/

C_eventp& setignore_(C_eventp& event, int x)
{
event.setignore(x);
return(event);
}

C_event_manipi setignore(int x)
{
return C_event_manipi(&setignore_, x);
}

/*****************************************************************************/
/*****************************************************************************/

C_eventp::C_eventp()
:valid_(0), event(0), size_(0), idx(0), num_ignore(0), to_ignore(0),
    num_copy(0), to_copy(0), trace(0)
{}

/*****************************************************************************/

C_eventp::~C_eventp()
{
delete[] to_ignore;
delete[] to_copy;
}

/*****************************************************************************/

void C_eventp::read_usr(FILE* f)
{
throw new C_program_error("C_eventp::read_usr: illegal");
}

/*****************************************************************************/

void C_eventp::write_usr(FILE* f)
{
int res;

if ((res=fwrite((char*)event, 4, event[0]+1, f))!=event[0]+1)
  {
  if (res==0)
    if (errno==0)
      cerr << "C_eventp::write_usr: ???" << endl;
    else
      cerr << "C_eventp::write_usr: " << strerror(errno);
  else
    cerr << "C_eventp::write_usr: ????" << endl;
  }
}

/*****************************************************************************/

C_eventp& C_eventp::operator <<(const C_subeventp& p)
{
throw new C_program_error("C_eventp::operator <<: illegal");
NORETURN((C_eventp&)p);
}

/*****************************************************************************/

C_eventp& C_eventp::operator >>(C_subeventp& subevent)
{
if (!valid_) throw new C_program_error("C_eventp::operator >>: no valid event");

subevent.clear();
if (num_ignore)
  {
  int weiter, id, i;
  do
    {
    if (!nextsubevent()) return(*this);
    id=event[idx];
    for (i=0, weiter=0; (i<num_ignore) && !weiter; i++)
        weiter=(id==to_ignore[i]);
    }
  while (weiter);
  subevent.store(event+idx);
  return(*this);
  }
else if (num_copy)
  {
  int weiter, id, i;
  do
    {
    if (!nextsubevent()) return(*this);
    id=event[idx];
    for (i=0, weiter=1; (i<num_copy) && weiter; i++)
        weiter=(id!=to_copy[i]);
    }
  while (weiter);
  subevent.store(event+idx);
  return(*this);
  }
else
  {
  if (nextsubevent()>0) subevent.store(event+idx);
  return(*this);
  }
}

/*****************************************************************************/

int C_eventp::evnr(void) const
{
if (!valid_) throw new C_program_error("C_eventp::evnr(): no valid event");
return(event[1]);
}

/*****************************************************************************/

int C_eventp::evnr(int nr)
{
throw new C_program_error("C_eventp::evnr(int): illegal");
NORETURN(0);
}

/*****************************************************************************/

int C_eventp::trignr(void) const
{
if (!valid_) throw new C_program_error("C_eventp::trignr(): no valid event");
return(event[2]);
}

/*****************************************************************************/

int C_eventp::trignr(int)
{
throw new C_program_error("C_eventp::trignr(int): illegal");
NORETURN(0);
}

/*****************************************************************************/

int C_eventp::sevnr(void) const
{
if (!valid_) throw new C_program_error("C_eventp::sevnr(): no valid event");
return(event[3]);
}

/*****************************************************************************/

int C_eventp::nextsubevent(void)
{
if (!valid_)
    throw new C_program_error("C_eventp::nextsubevent(): no valid event");
if (idx==0)
  idx=4;
else if (idx<size_)
  idx+=event[idx+1]+2;
return(idx<size_);
}

/*****************************************************************************/

int C_eventp::getsubevent(C_subeventp& sev, int id)
{
int idx;

if (!valid_)
    throw new C_program_error("C_eventp::getsubevent(): no valid event");
idx=4;
while ((idx<size_) && (id!=event[idx])) idx+=event[idx+1]+2;
if (idx<size_)
  {
  sev.store(event+idx);
  return 0;
  }
else
  return -1;
}

/*****************************************************************************/

C_eventp::operator int() const
{
return(valid_ && (idx<size_));
}

/*****************************************************************************/

void C_eventp::setcopy(int x)
{
if (num_ignore) throw new C_program_error("C_eventp::setcopy: num_ignore>0");
if (num_copy)
  {
  int i, *help;
  help=to_copy;
  to_copy=new int[num_copy+1];
  for (i=0; i<num_copy; i++) to_copy[i]=help[i];
  delete[] help;
  }
else
  {
  to_copy=new int[1];
  }
to_copy[num_copy++]=x;
}

/*****************************************************************************/

void C_eventp::setignore(int x)
{
if (num_copy) throw new C_program_error("C_eventp::setignore: num_copy>0");
if (num_ignore)
  {
  int i, *help;
  help=to_ignore;
  to_ignore=new int[num_ignore+1];
  for (i=0; i<num_ignore; i++) to_ignore[i]=help[i];
  delete[] help;
  }
else
  {
  to_ignore=new int[1];
  }
to_ignore[num_ignore++]=x;
}

/*****************************************************************************/
void C_eventp::store(const int* event_)
{
    event=(int*)event_;
    size_=event_[0]+1;
    valid_=1;
    idx=0;
    lnr=event[1];
}
/*****************************************************************************/

int C_eventp::empty(void) const
{
if (!valid_) throw new C_program_error("C_eventp::empty(): no valid event");
return(size_<=4);
}

/*****************************************************************************/

C_event::C_event()
{
event=0;
}

/*****************************************************************************/

C_event::~C_event()
{
delete[] event;
}

/*****************************************************************************/

C_eventp& C_event::operator <<(const C_subeventp& subevent)
{
int i;

if (!valid_)
  {
  event=new int[4+subevent.size()];
  if (event==0)
    {
    throw new C_unix_error(errno, "C_event::operator<<()");
    }
  event[0]=3;  // size
  event[1]=-1; // eventnummer (ungueltig)
  event[2]=0;  // trigger id
  event[3]=0;  // anzahl der subevents
  size_=4;
  valid_=1;
  idx=0;
  }
else
  {
  int* hevent=new int[size_+subevent.size()];
  if (hevent==0)
    {
    throw new C_unix_error(errno, "C_event::operator<<()");
    }
  for (i=0; i<size_; i++) hevent[i]=event[i];
  delete[] event;
  event=hevent;
  }
for (i=0; i<subevent.size(); i++) event[size_++]=subevent[i];
event[3]++;
event[0]=size_-1;
return(*this);
}

/*****************************************************************************/

void C_event::store(const int* event_)
{
int i;

delete[] event;
event=new int[event_[0]+1];
if (event==0)
  {
  valid_=0;
  throw new C_unix_error(errno, "C_event::store()");
  }
size_=event_[0]+1;
for (i=0; i<size_; i++) event[i]=event_[i];
valid_=1;
idx=0;
lnr=event[1];
}

/*****************************************************************************/

void C_event::read_usr(FILE* f)
{
int res, len;

if ((res=fread(&len, 4, 1, f))!=1)
  {
  if (errno==0)
    cerr << "C_event::read_usr: no data" << endl;
  else
    cerr << "C_event::read_usr: " << strerror(errno) << endl;
  valid_=0;
  return;
  }
delete[] event;
event=new int[len+1];
if (event==0)
  {
  valid_=0;
  throw new C_unix_error(errno, "C_event::read_usr()");
  }
event[0]=len;
if ((res=fread(event+1, 4, len, f))!=event[0])
  {
  if (res==0)
    if (errno==0)
      cerr << "C_event::read_usr: no data" << endl;
    else
      cerr << "C_event::read_usr: " << strerror(errno) << endl;
  else
    cerr << "C_event::read_usr: no data ????" << endl;
  valid_=0;
  return;
  }
size_=len+1;
valid_=1;
}

/*****************************************************************************/

int C_event::evnr(void) const
{
if (!valid_) throw new C_program_error("C_event::evnr(): no valid event");
return(event[1]);
}

/*****************************************************************************/

int C_event::evnr(int nr)
{
int onr;

if (valid_)
  {
  onr=event[1];
  event[1]=nr;
  }
else
  {
  event=new int[4];
  if (event==0)
    {
    throw new C_unix_error(errno, "C_event::evnr()");
    }
  onr=-1;
  event[0]=3;  // size
  event[1]=nr; // eventnummer
  event[2]=0;  // trigger id
  event[3]=0;  // anzahl der subevents
  size_=4;
  valid_=1;
  idx=0;
  }
lnr=event[1];
return(onr);
}

/*****************************************************************************/

int C_event::trignr(void) const
{
if (!valid_) throw new C_program_error("C_event::trignr(): no valid event");
return(event[2]);
}

/*****************************************************************************/

int C_event::trignr(int nr)
{
int onr;

if (valid_)
  {
  onr=event[2];
  event[2]=nr;
  }
else
  {
  event=new int[4];
  if (event==0)
    {
    throw new C_unix_error(errno, "C_event::evnr()");
    }
  onr=-1;
  event[0]=3;  // size
  event[1]=-1; // eventnummer (ungueltig)
  event[2]=nr; // trigger id
  event[3]=0;  // anzahl der subevents
  size_=4;
  valid_=1;
  idx=0;
  lnr=event[1];
  }
return(onr);
}

/*****************************************************************************/

int C_evput::settrace(int t)
{
int ot=trace;
trace=t;
return ot;
}

/*****************************************************************************/
/*****************************************************************************/
