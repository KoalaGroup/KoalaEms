/*
 * taperate.cc
 * 
 * created: 31.10.95
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <proc_communicator.hxx>
#include <proc_ved.hxx>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <client_defaults.hxx>
#include <versions.hxx>

VERSION("Sep 04 1997", __FILE__, __DATE__, __TIME__,
"$ZEL: taperate.cc,v 2.12 2007/04/10 00:01:05 wuestner Exp $")
#define XVERSION

C_VED* ved;
C_readargs* args;
int running, verbose, sense;
int num_do, *dolist;

/*****************************************************************************/

int readargs()
{
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK,
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("do", "do", -1, "dataout to be used", "dataout");
args->addoption("list", "list", false, "list all tape dataouts", "list");
args->addoption("interval", "i", 10, "interval in seconds", "interval");
args->addoption("verbose", "v", false, "verbose");
args->addoption("sense", "sense", false, "show sense keys");
args->addoption("vedname", 0, "", "Name des VED", "vedname");
return args->interpret(1);
}

/*****************************************************************************/
extern "C" {
#if defined(SA_HANDLER_VOID)
void alarmhand()
#else
void alarmhand(int sig)
#endif
{}
}
/*****************************************************************************/
extern "C" {
#if defined(SA_HANDLER_VOID)
void inthand()
#else
void inthand(int sig)
#endif
{
    running=0;
}
}
/*****************************************************************************/
int namelist_do(int* num, int** list)
{
    try {
        C_confirmation* conf=ved->GetNamelist(Object_domain, Dom_Dataout);
        *num=conf->buffer(1);
        *list=static_cast<int*>(malloc(*num*sizeof(int)));
        for (int i=0; i<*num; i++)
            (*list)[i]=conf->buffer(2+i);
        delete conf;
    } catch (C_error* e) {
        cerr<<(*e)<<endl;
        delete e;
        return -1;
    }
    return 0;
}
/*****************************************************************************/
int finddataout()
{
for (int d=0; d<num_do; d++)
  {
  C_data_io* addr;
  try
    {
    addr=ved->UploadDataout(dolist[d]);
    if (addr->addrtype()==Addr_Tape)
      {
      cout<<"dataout "<<dolist[d]<<": "<<(*addr)<<endl;
      delete addr;
      return dolist[d];
      }
    delete addr;
    }
  catch (C_error* e)
    {
    cout<<"dataout "<<dolist[d]<<": "<<(*e)<< endl;
    delete e;
    }
  }
return -1;
}
/*****************************************************************************/
void list_dataouts()
{
for (int d=0; d<num_do; d++)
  {
  C_data_io* addr;
  try
    {
    addr=ved->UploadDataout(dolist[d]);
    //if (addr->addrtype()==Addr_Tape)
      {
      cout<<"dataout "<<dolist[d]<<": "<<(*addr)<<endl;
      }
    delete addr;
    }
  catch (C_error* e)
    {
    cout<<"dataout "<<dolist[d]<<": "<<(*e)<< endl;
    delete e;
    }
  }
}
/*****************************************************************************/

void printsense(int key, int code, int qual)
{
if (key|| code|| qual) cout << hex << "(" << key << ","<< code << "," << qual
    << "): " << dec;
switch (key)
  {
  case 0:
    switch (code)
      {
      case 0:
        switch (qual)
          {
          case 0: break;
          case 1: cout << "filemark during read" << endl; break;
          case 2: cout << "logical end of tape" << endl; break;
          case 4: cout << "logical begin of tape" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x3b:
        switch (qual)
          {
          case 1: cout << "physicalor logical begin of tape" << endl; break;
          default: cout << "unknown" << endl; break;
          }
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 2:
    switch (code)
      {
      case 4:
        switch (qual)
          {
          case 0: cout << "not ready, cause unknown or tape not present" << endl; break;
          case 1: cout << "not ready, but in progress" << endl; break;
          case 2: cout << "not ready, tape motion command required" << endl; break;
          case 3: cout << "not ready, failed power-on self-test" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x3a:
        switch (qual)
          {
          case 0: cout << "not ready, tape not present" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 3:
    switch (code)
      {
      case 0:
        switch (qual)
          {
          case 0: cout << "undetermined medium error" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 3:
        switch (qual)
          {
          case 2: cout << "excessive write errors" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 9:
        switch (qual)
          {
          case 0: cout << "tracking error" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0xc:
        switch (qual)
          {
          case 0: cout << "hard write error" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x11:
        switch (qual)
          {
          case 1: cout << "hard read error" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x3b:
        switch (qual)
          {
          case 0: cout << "position error" << endl; break;
          case 2: cout << "physical end of tape" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 4:
    switch (code)
      {
      case 0:
        switch (qual)
          {
          case 0: cout << "undetermined hardware error" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 9:
        switch (qual)
          {
          case 0: cout << "track following error" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x40:
        switch (qual)
          {
          case 0x80: cout << "DPATH error" << endl; break;
          case 0x81: cout << "DPATH underrun" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x44:
        switch (qual)
          {
          case 0: cout << "Internal failure" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 5:
    switch (code)
      {
      case 0x1a:
        switch (qual)
          {
          case 0: cout << "parameter list error in CDB" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x20:
        switch (qual)
          {
          case 0: cout << "illegal operation code" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x24:
        switch (qual)
          {
          case 0: cout << "invalid field in CDB" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x25:
        switch (qual)
          {
          case 0: cout << "logical unit not supported" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x26:
        switch (qual)
          {
          case 0: cout << "invalid field in parameter list" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x3d:
        switch (qual)
          {
          case 0: cout << "invalid LUN" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x81:
        switch (qual)
          {
          case 0: cout << "mode mismatch" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x84:
        switch (qual)
          {
          case 0: cout << "??? compression mode mismatch ???" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 6:
    switch (code)
      {
      case 0x28:
        switch (qual)
          {
          case 0: cout << "tape load has occured, media may have been changed" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x29:
        switch (qual)
          {
          case 0: cout << "power-on reset, SCSI bus reset or device reset" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x2a:
        switch (qual)
          {
          case 1: cout << "mode select parameters have been changed" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x3f:
        switch (qual)
          {
          case 1: cout << "new microcode was loaded" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x5a:
        switch (qual)
          {
          case 1: cout << "??? eject button pressed ???" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 7:
    switch (code)
      {
      case 0x27:
        switch (qual)
          {
          case 0: cout << "tape is write protected" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 8:
    switch (code)
      {
      case 0:
        switch (qual)
          {
          case 5: cout << "end of data on read operation" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 0xb:
    switch (code)
      {
      case 0x45:
        switch (qual)
          {
          case 0: cout << "reselect failed" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x47:
        switch (qual)
          {
          case 0: cout << "SCSI bus parity error during write" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x48:
        switch (qual)
          {
          case 0: cout << "initiator detected error message during read" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      case 0x49:
        switch (qual)
          {
          case 0: cout << "invalid message from initiator" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  case 0xd:
    switch (code)
      {
      case 0x3b:
        switch (qual)
          {
          case 2: cout << "physical end of tape during write" << endl; break;
          default: cout << "unknown" << endl; break;
          }
        break;
      default: cout << "unknown" << endl; break;
      }
    break;
//-----------------------------------------------------------------------------
  default: cout << "unknown" << endl; break;
  }
}

/*****************************************************************************/

int rate(int dataout, int interval)
{
int first, lastok, lastpos, startpos;
struct timeval start, jetzt, vorher;
struct timezone tz;
const int maxsize=0x49AB40;

first=1;
lastok=0;
running=1;

while (running)
  {
  if (!first)
    {
    alarm(interval);
    sigpause(0);
    }
  try
    {
    C_confirmation* conf;
    int sec, usec;
    float zeit;

    conf=ved->GetDataoutStatus(dataout);

    gettimeofday(&jetzt, &tz);
    sec=jetzt.tv_sec-vorher.tv_sec;
    usec=jetzt.tv_usec-vorher.tv_usec;
    if (usec<0) {
        usec+=1000000; sec--;
    }
    zeit=static_cast<float>(sec)+static_cast<float>(usec)/1000000.0;
    vorher=jetzt;

    switch (static_cast<DoRunStatus>(conf->buffer(2)))
      {
      case Do_running: break;
      case Do_neverstarted: cout << "ready to run; "; break;
      case Do_done: cout << "no more data; "; break;
      case Do_error: cout << "not ready; "; break;
      default: cout << "unknown status " << conf->buffer(2) << "; "; break;
      }
    if (conf->buffer(3)) cout << "disabled; ";

    if (conf->buffer(1))
      {
      cout << "error " << conf->buffer(1) << endl;
      }
    else if (conf->size()<8)
      cout << "wrong format of confirmation; size=" << conf->size() << endl;
    else
      {
      int diff;
      float byterate;

      if (conf->buffer(1)) cout << "error " << conf->buffer(1) << "; ";
      cout << "remaining tape " << conf->buffer(4) << " kByte; ";
      cout << "errorcount " << conf->buffer(5);
      if (first)
        {
        startpos=conf->buffer(4);
        start=jetzt;
        cout << endl;
        }
      else
        {
        cout << "; ";
        diff=lastpos-conf->buffer(4);
        byterate=float(diff)/float(zeit);
        cout.precision(2);
        cout.setf(ios::fixed, ios::floatfield);
        cout.width(9);
        cout << byterate << " kByte/s" << endl;
        }
      lastpos=conf->buffer(4);
      if (conf->buffer(6))
        {
        int flags2, flags19, flags20, flags21, i, komma;
        const char* senses2[]=
          {
          "reserved 2/4",
          "illegal length indicator",
          "end of medium",
          "filemark"
          };
        const char* senses19[]=
          {
          "logical begin of tape",
          "tape not present",
          "tape motion error",
          "error counter overflow",
          "media error",
          "formatted buffer parity error",
          "SCSI bus parity error",
          "power fail"
          };
        const char* senses20[]=
          {
          "formatter error",
          "servo system error",
          "write error 1",
          "underrun error",
          "filemark error",
          "write protected",
          "tape mark detect error",
          "reserved 20/7",
          };
        const char* senses21[]=
          {
          "write splice error overshot",
          "write splice error blank",
          "physical end of tape",
          "reserved 21/3",
          "reserved 21/4",
          "reserved 21/5",
          "reserved 21/6",
          "reserved 21/7",
          };

        komma=0;
//        if (sense)
//          {
//          sensekey=(conf->buffer(6)>>24)&0xf;
//          if (sensekey) {cout << "sense key=" << sensekey; komma=1;}
//          }
        flags2=(conf->buffer(6)>>28)&0xf;
        flags19=(conf->buffer(6)>>16)&0xff;
        flags20=(conf->buffer(6)>>8)&0xff;
        flags21=conf->buffer(6)&0xff;
        for (i=0; flags2; i++, flags2>>=1)
          {
          if (flags2&1) {if (komma) cout << ", "; cout << senses2[i]; komma=1;}
          }
        for (i=0; flags19; i++, flags19>>=1)
          {
          if (flags19&1)
              {if (komma) cout << ", "; cout << senses19[i]; komma=1;}
          }
        for (i=0; flags20; i++, flags20>>=1)
          {
          if (flags20&1)
              {if (komma) cout << ", "; cout << senses20[i]; komma=1;}
          }
        for (i=0; flags21; i++, flags21>>=1)
          {
          if (flags21&1)
              {if (komma) cout << ", "; cout << senses21[i]; komma=1;}
          }
        if (komma) cout << endl;
        }
//    if (sense && conf->buffer(7))
//      {
//      cout << "add. sense code=" << ((conf->buffer(7)>>8)&0xff);
//      cout << ", add. sense code qualifier=" << (conf->buffer(7)&0xff) << endl;
//      }
      printsense((conf->buffer(6)>>24)&0xf, (conf->buffer(7)>>8)&0xff,
          conf->buffer(7)&0xff);
      if (verbose && !(conf->buffer(6)&0x30000))
        {
        int saved;
        float percent;

        saved=maxsize-lastpos;
        cout << saved << " kByte saved (";
        percent=static_cast<float>(saved)/static_cast<float>(maxsize)*100.0;
        cout.precision(1);
        cout.setf(ios::fixed, ios::floatfield);
        cout.width(5);
        cout << percent << "%); " << endl;
        }
      }
    }
  catch (C_error* error)
    {
    cerr << (*error) << endl;
    delete error;
    }
  first=0;
  }
return(0);
}

/*****************************************************************************/

int main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

verbose=args->getboolopt("verbose");
sense=args->getboolopt("sense");

if (args->isdefault("vedname"))
  {
  cerr << "vedname expected" << endl;
  return(0);
  }
try
  {
  if (!args->isdefault("hostname") || !args->isdefault("port"))
    {
    communication.init(args->getstringopt("hostname"),
        args->getintopt("port"));
    }
  else
    {
    communication.init(args->getstringopt("sockname"));
    }
  ved=new C_VED(args->getstringopt("vedname"));
  }
catch(C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  return(0);
  }

struct sigaction vec, ovec;

if (sigemptyset(&vec.sa_mask)==-1)
  {
  cerr << "sigemptyset: " << strerror(errno) << endl;
  return(-1);
  }
vec.sa_flags=0;
vec.sa_handler=alarmhand;
if (sigaction(SIGALRM, &vec, &ovec)==-1)
  {
  cerr << "sigaction: " << strerror(errno) << endl;
  return(-1);
  }
vec.sa_handler=inthand;
if (sigaction(SIGINT, &vec, &ovec)==-1)
  {
  cerr << "sigaction: " << strerror(errno) << endl;
  return(-1);
  }

if (namelist_do(&num_do, &dolist)<0) return 0;
if (num_do==0)
  {
  cout<<"no dataouts available."<<endl;
  return 0;
  }
if (args->getboolopt("list"))
  {
  list_dataouts();
  return 0;
  }
int dataout=args->getintopt("do");
if (dataout<0)
  {
  dataout=finddataout();
  if (dataout<0)
    {
    cerr<<"cannot find dataout with tape."<<endl;
    return 0;
    }
  }
rate(dataout, args->getintopt("interval"));

delete ved;
communication.done();
cout << "ende" << endl;
return 0;
}

/*****************************************************************************/
/*****************************************************************************/
