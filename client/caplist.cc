/*
 * caplist.cc
 * 
 * created: 18.11.94
 * 25.03.1998 PW: adapded for <string>
 * 29.06.1998 PW: adapted for STD_STRICT_ANSI
 * 07.Apr.2003 PW: cxxcompat.hxx used
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <xdrstrdup.hxx>
#include <client_defaults.hxx>
#include <versions.hxx>

VERSION("Mar 25 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: caplist.cc,v 2.11 2007/04/11 13:38:44 wuestner Exp $")
#define XVERSION

C_VED* cc;
C_instr_system* is;
C_readargs* args;
int verbose;

/*****************************************************************************/

int readargs()
{
args->addoption("all", "a", false, "all infos", "");
args->addoption("verbose", "v", false, "verbose", "");
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
args->addoption("trig", "trig", false, "trigprocs", "");
args->addoption("help", "help", false, "this text", "");
return args->interpret(1);
}

/*****************************************************************************/

int cap_list(Capabtyp typ)
{
int all;

if (verbose) cout << "cap_list:" << endl;
all=args->getboolopt("all");

try
  {
  cc->version_separator('.', Capab_listproc);
#ifndef __GNUC__
  if (!all && isatty(1))
    {
    int n;
    struct winsize ttysize;
    int width, columns;

    int num=cc->numprocs(typ);
    STRING* list=new STRING[num];
    for (int i=0; i<num; i++) list[i]=cc->procname(i, typ);
    int max=0;
    for (int j=0; j<num; j++) if ((n=list[j].length())>max) max=n;
    if (ioctl(0, TIOCGWINSZ, (char*)&ttysize)==0)
      {
      width=ttysize.ws_col;
      if (width==0) width=80;
      }
    else
      {
      cerr << "ioctl: errno=" << errno << endl;
      width=80;
      }
    columns=width/(max+2);
    if (columns<1) columns=1;
    cout.setf(ios::left, ios::adjustfield);
    for (int k=0; k<num; k++)
      {
      cout << setw(max) << list[k] << "  ";
      if (((k+1)%columns==0) || (k+1==num)) cout << endl;
      }
    delete[] list;
    }
  else
#endif
    {
    int num=cc->numprocs(typ);
    for (int id=0; id<num; id++)
      {
      STRING s=cc->procname(id, typ);
      if (all)
        {
        cout.setf(ios::right, ios::adjustfield);
        cout << setw(3) << id << ": ";
        cout.setf(ios::left, ios::adjustfield);
        }
      cout << s << endl;
      }
    }
  }
catch(C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(-1);
  }
return(0);
}

/*****************************************************************************/

int main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

verbose=args->getboolopt("verbose");
if (args->isdefault("vedname"))
  {
  cerr << "vedname expected" << endl;
  return(0);
  }
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
  cc=new C_VED(args->getstringopt("vedname"));
  if (verbose) cout << "ok." << endl;
  }
catch(C_error* e)
  {
  cerr << (*e) << endl;
  return(0);
  }

is=cc->is0();
if (is==0)
  {
  cerr<<"VED "<<cc->name()<<" has no procedures."<<endl;
  }
else
  {
  cap_list(args->getboolopt("trig")?Capab_trigproc:Capab_listproc);
  }
delete cc;
communication.done();
}

/*****************************************************************************/
/*****************************************************************************/
