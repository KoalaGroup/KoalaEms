/*
 * events.cc
 * 
 * created 27.01.95 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errors.hxx>
#include "events.hxx"
#include "swap_int.h"
#include "compat.h"
#include <versions.hxx>

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: evoutput.cc,v 1.9 2010/09/04 21:19:49 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_evoutput::C_evoutput(const STRING& name, int maxevent)
:C_evput(name, maxevent), swap_(0), buffer(0)
{}

/*****************************************************************************/

C_evoutput::C_evoutput(const char* name, int maxevent)
:C_evput(name, maxevent), swap_(0), buffer(0)
{}

/*****************************************************************************/

C_evoutput::~C_evoutput()
{
delete[] buffer;
}

/*****************************************************************************/

int C_evoutput::swap(int swap)
{
int sw=swap_;
swap_=swap;
if (swap_ && !buffer) buffer=new int[maxevent_];
return(sw);
}

/*****************************************************************************/

C_evpoutput::C_evpoutput(const STRING& name, int maxevent)
:C_evoutput(name, maxevent)
{
if (name=="-")
  path=1;
else
  {
  path=open(name.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0640);
  if (path==-1)
    {
    STRING msg("open tape \"");
    msg+=name;
    msg+="\" for output";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evpoutput::C_evpoutput(const char* name, int maxevent)
:C_evoutput(name, maxevent)
{
if (strcmp(name, "-")==0)
  path=1;
else
  {
  path=open((char*)name, O_WRONLY|O_CREAT|O_TRUNC, 0640);
  if (path==-1)
    {
    STRING msg("open file \"");
    msg+=name;
    msg+="\" for output";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evpoutput::C_evpoutput(int pfad, int maxevent)
:C_evoutput("unknown path", maxevent), path(pfad)
{
}

/*****************************************************************************/

C_evpoutput::~C_evpoutput()
{
close(path);
}

/*****************************************************************************/

int C_evpoutput::xsend(const int* buf, int len)
{
int dort, restlen;
char *bufptr;

if (trace)
  {
  cerr << "C_evpoutput::xsend: size="<<len<<endl<<hex;
  for (int i=0; i<len; i++) cerr << ' ' << buf[i];
  cerr << dec << endl;
  }
restlen=len*sizeof(int);
bufptr=(char*)buf;
while (restlen)
  {
  if (trace)
    {
    cerr << "write("<<path<<", "<<(void*)bufptr<<", {"<<hex;
    for (int i=0; i<restlen/4; i++)
      {
      cerr << ((int*)bufptr)[i];
      if (i+1<restlen/4) cerr << ", ";
      }
    cerr <<dec<< "}, "<<restlen<<")" << endl;
    }
  dort=write(path, bufptr, restlen);
  if (dort>0)
    {
    bufptr+=dort;
    restlen-=dort;
    }
  else if (dort==0)
    {
    throw new C_program_error("C_evpoutput::xsend: ???");
    }
  else
    {
    if (errno!=EINTR)
      {
      throw new C_unix_error(errno, "C_evpoutput::xsend");
      }
    }
  }
return(0);

}

/*****************************************************************************/

C_evoutput& C_evpoutput::operator <<(const C_eventp& event)
{
int size;
size=event.size();
if (swap_)
  {
  int i;
  for (i=0; i<size; i++) buffer[i]=swap_int(event[i]);
  xsend(buffer, size);
  }
else
  {
  xsend(event.buffer(), size);
  }
return(*this);
}

/*****************************************************************************/

C_evtoutput::C_evtoutput(const STRING& name, int maxevent)
:C_evoutput(name, maxevent)
{
path=open(name.c_str(), O_WRONLY|O_CREAT, 0640);
if (path==-1)
  {
  STRING msg("open tape \"");
  msg+=name;
  msg+="\" for output";
  throw new C_unix_error(errno, msg);
  }
buffer=new int[maxevent_];
idx=0;
}

/*****************************************************************************/

C_evtoutput::C_evtoutput(const char* name, int maxevent)
:C_evoutput(name, maxevent)
{
path=open((char*)name, O_WRONLY|O_CREAT, 0640);
if (path==-1)
  {
  STRING msg("open tape \"");
  msg+=name;
  msg+="\" for output";
  throw new C_unix_error(errno, msg);
  }
buffer=new int[maxevent_];
idx=0;
}

/*****************************************************************************/

C_evtoutput::C_evtoutput(int pfad, int maxevent)
:C_evoutput("unknown tape", maxevent)
{
path=pfad;
buffer=new int[maxevent_];
idx=0;
}

/*****************************************************************************/

C_evtoutput::~C_evtoutput()
{
flush();
close(path);
}

/*****************************************************************************/

int C_evtoutput::writerecord(const int* buf, int len)
{
int res;

if (trace) cerr << "trace in C_evtoutput noch nicht implementiert" << endl;
res=write(path, (char*)buf, len);
if (res==-1)
  {
  throw new C_unix_error(errno, "C_evtoutput::writerecord");
  }
else
  return(res);
}

/*****************************************************************************/

C_evoutput& C_evtoutput::operator <<(const C_eventp& event)
{
int size, i;

size=event.size();
if (idx+size>maxevent_) flush();

for (i=0; i<size; i++) buffer[idx++]=event[i];
return(*this);
}

/*****************************************************************************/

void C_evtoutput::flush()
{
if (idx==0) return;
if (swap_)
  {
  int i;
  for (i=0; i<idx; i++) buffer[i]=htonl(buffer[i]);
  }
writerecord(buffer, idx*4);
idx=0;
return;
}

/*****************************************************************************/

int C_evtoutput::swap(int swap)
{
int sw=swap_;
swap_=swap;
return(sw);
}

/*****************************************************************************/

C_evfoutput::C_evfoutput(int path, int maxevent)
:C_evoutput("unknown file", maxevent)
{
file=fdopen(path, "w");
if (file==0)
  {
  OSTRINGSTREAM st;
  st<<"fdopen("<<path<<")"<<ends;
  throw new C_unix_error(errno, st);
  }
}

/*****************************************************************************/

C_evfoutput::C_evfoutput(const char* name, int maxevent)
:C_evoutput(name, maxevent)
{
if (strcmp(name, "-")==0)
  file=stdout;
else
  {
  file=fopen(name, "w");
  if (file==0)
    {
    STRING msg("open file \"");
    msg+=name;
    msg+="\" for output";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evfoutput::C_evfoutput(const STRING& name, int maxevent)
:C_evoutput(name, maxevent)
{
if (name=="-")
  file=stdout;
else
  {
  file=fopen(name.c_str(), "w");
  if (file==0)
    {
    STRING msg("open file \"");
    msg+=name;
    msg+="\" for output";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evfoutput::~C_evfoutput()
{
fclose(file);
}

/*****************************************************************************/

int C_evfoutput::xsend(const int* buf, int len)
{
int dort;

if (trace) cerr << "trace in C_evfoutput noch nicht implementiert" << endl;
errno=0;
if ((dort=fwrite(buf, 4, len, file))!=len)
  {
  if (dort==0)
    if (errno==0)
      throw new C_program_error("C_evfoutput::xsend: ???");
    else
      throw new C_unix_error(errno, "C_evfoutput::xsend");
  else
    throw new C_program_error("C_evfoutput::xsend: ????");
  }
return(0);
}

/*****************************************************************************/

C_evoutput& C_evfoutput::operator <<(const C_eventp& event)
{
int size;
size=event.size();
if (swap_)
  {
  int i;
  for (i=0; i<size; i++) buffer[i]=htonl(event[i]);
  xsend(buffer, size);
  }
else
  xsend(event.buffer(), size);
return(*this);
}

/*****************************************************************************/

C_evoutput& flush(C_evoutput& outp)
{
outp.flush();
return(outp);
}

/*****************************************************************************/

C_evoutput& C_evoutput::operator<<(evoutput_manip f)
{
return(f(*this));
}

/*****************************************************************************/
/*****************************************************************************/
