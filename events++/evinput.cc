/*
 * evinput.cc
 * 
 * created 27.01.95 PW
 */

#include "config.h"
#include <string>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errors.hxx>
#include "events.hxx"
#include "swap_int.h"
#include <versions.hxx>

VERSION("2014-07-14", __FILE__, __DATE__, __TIME__,
"$ZEL: evinput.cc,v 1.11 2014/07/14 16:18:17 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_evinput::C_evinput(const char* name, int maxevent)
:C_evput(name, maxevent), status(1), lastwend(unknown), maxexpect(0)
{
eventbuf=new int[maxevent_];
}

/*****************************************************************************/

C_evinput::C_evinput(const string& name, int maxevent)
:C_evput(name, maxevent), status(1), lastwend(unknown), maxexpect(0)
{
eventbuf=new int[maxevent_];
}

/*****************************************************************************/

C_evinput::~C_evinput()
{
delete[] eventbuf;
}

/*****************************************************************************/

void C_evinput::reset()
{
if (!fatal()) status=1;
}

/*****************************************************************************/

void C_evinput::setmaxexpect(int max)
{
maxexpect=max;
}

/*****************************************************************************/

C_evpinput::C_evpinput(const char* name, int maxevent)
:C_evinput(name, maxevent)
{
if (strcmp(name, "-")==0)
  path=0;
else
  {
  path=open(name, O_RDONLY, 0);
  if (path==-1)
    {
    string msg("open file \"");
    msg+=name;
    msg+="\" for input";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evpinput::C_evpinput(const string& name, int maxevent)
:C_evinput(name, maxevent)
{
if (name=="-")
  path=0;
else
  {
  path=open(name.c_str(), O_RDONLY, 0);
  if (path==-1)
    {
    string msg("open tape \"");
    msg+=name;
    msg+="\" for input";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evpinput::C_evpinput(int pfad, int maxevent)
:C_evinput("unknown path", maxevent), path(pfad)
{
}

/*****************************************************************************/

C_evpinput::~C_evpinput()
{
close(path);
}

/*****************************************************************************/

void C_evpinput::xrecv(int* buf, int len)
{
int da, restlen;
char *bufptr;

restlen=len*sizeof(int);
bufptr=reinterpret_cast<char*>(buf);
while (restlen)
  {
  da=read(path, bufptr, restlen);
  if (da>0)
    {
    bufptr+=da;
    restlen-=da;
    }
  else if (da==0)
    throw new C_program_error("no more data");
  else
    {
    if (errno!=EINTR)
      throw new C_unix_error(errno, "C_evpinput::xrecv");
    }
  }
}

/*****************************************************************************/

C_evinput& C_evpinput::operator >>(C_eventp& event)
{
int i;
int* ptr=0;
wendmod wenden;

if (status==0) return(*this);

event.clear();

try
  {
  xrecv(eventbuf, 4);
  }
catch (C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  status=0;
  return(*this);
  }

try
  {
  if (eventbuf[3]==0)
    wenden=eventbuf[0] & 0xffff0000?yes:no;
  else
    wenden=eventbuf[3] & 0xffff0000?yes:no;

  if ((lastwend!=wenden) && (lastwend!=unknown))
    {
    cerr << "Warning: endien changed; evnum=" << hex << eventbuf[1] << dec
        << endl;
    }
  lastwend=wenden;
  if (wenden==yes)
    {
    ptr=eventbuf;
    for (i=0; i<4; i++, ptr++) *ptr=swap_int(*ptr);
    }

  if (eventbuf[0]>maxevent_)
    {
    ostringstream st;
    st<<"C_evpinput::operator>>: event too big"<<endl;
    st<<"  size="<<eventbuf[0]<<" max="<<maxevent_<<ends;
    throw new C_program_error(st);
    }

  if (maxexpect && (eventbuf[0]>maxexpect))
    {
    cerr << "Warning: event " << eventbuf[1] << "; size=" << eventbuf[0]
        << endl;
    }
  xrecv(eventbuf+4, eventbuf[0]-3);

  if (wenden==yes)
    for (i=4; i<=eventbuf[0]; i++, ptr++) *ptr=swap_int(*ptr);
  }
catch (C_error* error)
  {
  cerr << (*error) << "; last event (" << eventbuf[1] << ") was not complete"
      << endl;
  delete error;
  status=0;
  return(*this);
  }

event.store(eventbuf);
status=1;
return(*this);
}

/*****************************************************************************/

int C_evpinput::fatal(void) const
{
return(status==0);
}

/*****************************************************************************/

C_evtinput::C_evtinput(const char* name, int maxevent)
:C_evinput(name, maxevent), blocksize(0), idx(0), filemark_(0), fatal_(0)
{
path=open(name, O_RDONLY, 0);
if (path==-1)
  {
  string msg("open tape \"");
  msg+=name;
  msg+="\" for input";
  throw new C_unix_error(errno, msg);
  }
}

/*****************************************************************************/

C_evtinput::C_evtinput(const string& name, int maxevent)
:C_evinput(name, maxevent), blocksize(0), idx(0), filemark_(0), fatal_(0)
{
path=open(name.c_str(), O_RDONLY, 0);
if (path==-1)
  {
  string msg("open tape \"");
  msg+=name;
  msg+="\" for input";
  throw new C_unix_error(errno, msg);
  }
}

/*****************************************************************************/

C_evtinput::C_evtinput(int pfad, int maxevent)
:C_evinput("unknown tape", maxevent)
{
path=pfad;
}

/*****************************************************************************/

C_evtinput::~C_evtinput()
{
close(path);
}

/*****************************************************************************/

int C_evtinput::readrecord(int* buf)
{
int res;

res=read(path, buf, maxevent_*sizeof(int));
if (res==-1)
  {
  throw new C_unix_error(errno, "read tape");
  }
else
  return(res);
}

/*****************************************************************************/

C_evinput& C_evtinput::operator >>(C_eventp& event)
{
int *eventptr, *ptr;
int cblocksize, i;
wendmod wenden;

if (status==0) return(*this);

event.clear();

if (idx>=blocksize)
  {
  try
    {
    if ((cblocksize=readrecord(eventbuf))==0)
      {
      status=0;
      filemark_=1;
      return(*this);
      }
    }
  catch (C_error* error)
    {
//    cerr << error << endl;
    fatal_=1;
    status=0;
//    return(*this);
    throw;
    };
  if (cblocksize%4!=0)
    {
    cerr << "C_evtinput::operator>>: wrong blocksize" << endl;
    status=0;
    return(*this);
    }
  blocksize=cblocksize/4;
  if (eventbuf[3]==0)
    wenden=eventbuf[0] & 0xffff0000?yes:no;
  else
    wenden=eventbuf[3] & 0xffff0000?yes:no;

  if ((lastwend!=wenden) && (lastwend!=unknown))
    {
    cerr << "Warning: endien changed; evnum=" << hex << eventbuf[1] << dec
        << endl;
    }
  lastwend=wenden;
  if (wenden==yes)
    {
    ptr=eventbuf;
    for (i=0; i<blocksize; i++, ptr++) *ptr=swap_int(*ptr);
    }

  idx=0;
  }
eventptr=eventbuf+idx;
if (maxexpect && (eventptr[0]>maxexpect))
  {
  cerr << "Warning: event " << eventptr[1] << "; size=" << eventptr[0]
      << endl;
  }
idx+=eventptr[0]+1;
event.store(eventptr);
status=1;
return(*this);
}

/*****************************************************************************/

int C_evtinput::fatal(void) const
{
return(fatal_);
}

/*****************************************************************************/

int C_evtinput::windfile(int num)
{
int res;
struct mtop mt_com;

blocksize=0;
if (num==0)
  return(0);
else if(num<0)
  {
  mt_com.mt_op=MTBSF;
  mt_com.mt_count=-num;
  }
else
  {
  mt_com.mt_op=MTFSF;
  mt_com.mt_count=num;
  }
res=ioctl(path, MTIOCTOP, &mt_com);
if (res<0)
  {
  cerr << "windfile: res=" << res << "; " << strerror(errno) << endl;
  return(-1);
  }
return(0);
}

/*****************************************************************************/

int C_evtinput::windrecord(int num)
{
int res;
struct mtop mt_com;

blocksize=0;
if (num==0)
  return(0);
else if(num<0)
  {
  mt_com.mt_op=MTBSR;
  mt_com.mt_count=-num;
  }
else
  {
  mt_com.mt_op=MTFSR;
  mt_com.mt_count=num;
  }
res=ioctl(path, MTIOCTOP, &mt_com);
if (res<0) throw new C_unix_error(errno, "windrecord");
return(0);
}

/*****************************************************************************/

int C_evtinput::rewind()
{
int res;
struct mtop mt_com;

blocksize=0;
mt_com.mt_op=MTREW;
mt_com.mt_count=1;
res=ioctl(path, MTIOCTOP, &mt_com);
if (res<0)
  {
  cerr << "rewind: res=" << res << "; " << strerror(errno) << endl;
  return(-1);
  }
return(0);
}

/*****************************************************************************/

C_evfinput::C_evfinput(const char* name, int maxevent)
:C_evinput(name, maxevent)
{
if (strcmp(name, "-")==0)
  file=stdin;
else
  {
  file=fopen(name, "r");
  if (file==0)
    {
    string msg("open file \"");
    msg+=name;
    msg+="\" for input";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evfinput::C_evfinput(const string& name, int maxevent)
:C_evinput(name, maxevent)
{
if (name=="-")
  file=stdin;
else
  {
  file=fopen(name.c_str(), "r");
  if (file==0)
    {
    string msg("open file \"");
    msg+=name;
    msg+="\" for input";
    throw new C_unix_error(errno, msg);
    }
  }
}

/*****************************************************************************/

C_evfinput::C_evfinput(int path, int maxevent)
:C_evinput("unknown file", maxevent)
{
file=fdopen(path, "r");
if (file==0)
  {
  ostringstream st;
  st<<"fdopen("<<path<<")"<<ends;
  throw new C_unix_error(errno, st);
  }
}

/*****************************************************************************/

C_evfinput::~C_evfinput()
{
if (file!=stdin) fclose(file);
}

/*****************************************************************************/

void C_evfinput::xrecv(int* buf, int len)
{
int res;

errno=0;
if ((res=fread(buf, 4, len, file))!=len)
  {
  if (res==0)
    if (errno==0)
      throw new C_program_error("no more data");
    else
      throw new C_unix_error(errno, "C_evfinput::xrecv");
  else
    throw new C_program_error("no more data");
  }
}

/*****************************************************************************/

C_evinput& C_evfinput::operator >>(C_eventp& event)
{
int i;
int* ptr=0;
wendmod wenden;

if (status==0) return(*this);

event.clear();

try
  {
  xrecv(eventbuf, 4);
  }
catch (C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  status=0;
  return(*this);
  }

try
  {
  if (eventbuf[3]==0)
    wenden=eventbuf[0] & 0xffff0000?yes:no;
  else
    wenden=eventbuf[3] & 0xffff0000?yes:no;

  if ((lastwend!=wenden) && (lastwend!=unknown))
    {
    cerr << "Warning: endien changed; evnum=" << hex << eventbuf[1] << dec
        << endl;
    }
  lastwend=wenden;
  if (wenden==yes)
    {
    ptr=eventbuf;
    for (i=0; i<4; i++, ptr++) *ptr=swap_int(*ptr);
    }

  if (eventbuf[0]>maxevent_)
    {
    ostringstream st;
    st<<"C_evfinput::operator>>: event too big"<<endl;
    st<<"  size="<<eventbuf[0]<<" max="<<maxevent_<<ends;
    throw new C_program_error(st);
    }

  if (maxexpect && (eventbuf[0]>maxexpect))
    {
    ostringstream st;
    st<<"C_evfinput::operator>>: event too big"<<endl;
    st<<"  size="<<eventbuf[0]<<" max="<<maxexpect<<ends;
    throw new C_program_error(st);
    }
  xrecv(eventbuf+4, eventbuf[0]-3);

  if (wenden==yes)
    for (i=4; i<=eventbuf[0]; i++, ptr++) *ptr=swap_int(*ptr);
  }
catch (C_error* error)
  {
  cerr << (*error) << "; last event (" << eventbuf[1] << ") was not complete"
      << endl;
  delete error;
  status=0;
  return(*this);
  }

event.store(eventbuf);
status=1;
return(*this);
}

/*****************************************************************************/

int C_evfinput::fatal(void) const
{
return(status==0);
}

/*****************************************************************************/
/*****************************************************************************/
