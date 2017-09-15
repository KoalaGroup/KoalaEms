/*
 * vednames.cc
 * 
 * created: 18.11.94
 * 25.03.1998 PW: adapded for <string>
 * 29.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <errorcodes.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <inbuf.hxx>
#include <ved_addr.hxx>
#include <client_defaults.hxx>
#include <versions.hxx>

VERSION("Mar 25 1997", __FILE__, __DATE__, __TIME__,
"$ZEL: vednames.cc,v 2.13 2006/11/27 11:00:05 wuestner Exp $")
#define XVERSION

C_instr_system* is;
C_readargs* args;
int verbose;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "v", false, "verbose", "");
args->addoption("addrs", "a", false, "show addresses also", "");
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK,
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("vedname", 0, "commu", "Name des VED", "vedname");
return args->interpret(1);
}

/*****************************************************************************/

int list_of_vednames()
{
if (verbose) cout << "list_of_vednames:" << endl;
try
  {
  C_confirmation* conf;
  int i, num;
  int istty, columns;

  conf=is->command("VEDnames");
  C_inbuf inbuf(conf->buffer(), conf->size());
  inbuf++;

  istty=isatty(1);

  if (istty)
    {
    struct winsize ttysize;
    int res, width;
    res=ioctl(0, TIOCGWINSZ, reinterpret_cast<char*>(&ttysize));
    if (res==0)
      {
      width=ttysize.ws_col;
      if (width==0) width=80;
      }
    else
      {
      cerr << "ioctl: errno=" << errno << endl;
      width=80;
      }
    columns=width/16;
    if (columns<1) columns=1;
    cout.setf(ios::left, ios::adjustfield);
    }
  inbuf >> num;
  for (i=0; i<num; i++)
    {
    STRING s;
    inbuf >> s;
    if (istty)
      {
      cout << setw(16) << s;
      if (((i+1)%columns==0) || (i+1==num)) cout << endl;
      }
    else
      cout << s << endl;
    }
  delete conf;
  }
catch(C_error* e)
  {
  cerr << endl << (*e) << endl;
  delete e;
  return(-1);
  }
return(0);
}

/*****************************************************************************/

int list_of_addresses()
{
if (verbose) cout << "list_of_addresses:" << endl;
try
  {
  C_confirmation* conf0;
  int i, num;

  conf0=is->command("VEDnames");
  C_inbuf inbuf0(conf0->buffer(), conf0->size());
  inbuf0++; // fehlercode ueberspringen
  inbuf0 >> num;

  cout.setf(ios::left, ios::adjustfield);
  for (i=0; i<num; i++)
    {
    STRING s;
    C_confirmation* conf1;
    inbuf0 >> s;

    cout << setw(16) << s << flush;
#ifdef USE_STRING_H
    conf1=is->command("VEDaddress", s);
#else
    conf1=is->command("VEDaddress", s.c_str());
#endif
    C_inbuf inbuf1(conf1->buffer(), conf1->size());
    inbuf1++; // fehlercode ==0
    if (inbuf1.getint()==0)
      {
      cout << "ist verschwunden." << endl;
      }
    else
      {
      C_VED_addr* addr;
      addr=extract_ved_addr(inbuf1);
      cout << addr << endl;
/*
        switch (conf1->buffer[1])
          {
          case 0: cout << "notvalid" << endl; break;
          case 1: cout << "builtin" << endl; break;
          case 2:
            {
            char* s;
            cout << "unix          : ";
            s=xdrstrdup(conf1->buffer+2);
            cout << s << endl;
            delete s;
            }
            break;
          case 3:
            {
            char* s; int* ptr;
            cout << "inet          : ";
            ptr=xdrstrdup(&s, conf1->buffer+2);
            cout << setw(12) << s << ": " << *ptr << endl;
            delete s;
            }
            break;
          case 4:
            {
            char* s; int *ptr, num, i;
            cout << "inet with path: ";
            ptr=xdrstrdup(&s, conf1->buffer+2);
            cout << setw(12) << s << ": " << *ptr++;
            delete s;
            num=*ptr++;
            for (i=0; i<num; i++)
              {
              ptr=xdrstrdup(&s, conf1->buffer+2);
              cout << " " << s;
              delete s;
              }
            cout << endl;
            }
            break;
          case 5: cout << "vic" << endl; break;
          default: cout << conf1->buffer[1] << endl; break;
          }
        }
*/
      delete addr;
      }
    delete conf1;
    }
  delete conf0;
  }
catch(C_error* e)
  {
  cerr << endl << (*e) << endl;
  delete e;
  return(-1);
  }
return(0);
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
C_VED* commu;
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);
verbose=args->getboolopt("verbose");
try
  {
  if (!args->isdefault("hostname") || !args->isdefault("port"))
    {
    if (verbose)
        cout << "use host " << args->getstringopt("hostname") << ", port "
        << args->getintopt("port") << " for communication: " << flush;
    communication.init(args->getstringopt("hostname"),
        args->getintopt("port"));
    }
  else //if (!args->isdefault("sockname"))
    {
    if (verbose)
        cout << "use socket " << args->getstringopt("sockname")
        << " for communication: " << flush;
    communication.init(args->getstringopt("sockname"));
    }
  if (verbose) cout << "ok." << endl;

  if (verbose)cout << "open " << args->getstringopt("vedname") << ": " << flush;
  commu=new C_VED(args->getstringopt("vedname"));
  if (verbose) cout << "ok." << endl;
  }
catch(C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(0);
  }
is=commu->is0();
if (is==0)
  {
  cerr<<"VED "<<commu->name()<<" has no procedures."<<endl;
  }
else
  {
  if (args->getboolopt("addrs"))
    list_of_addresses();
  else
    list_of_vednames();
  }
delete commu;
communication.done();
}

/*****************************************************************************/
/*****************************************************************************/
