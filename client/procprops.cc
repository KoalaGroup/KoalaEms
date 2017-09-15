/*
 * procprops.cc  
 * created: 22.11.94 PW
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

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: procprops.cc,v 2.12 2007/04/18 20:10:56 wuestner Exp $")
#define XVERSION

C_VED* cc;
C_instr_system* is;
C_readargs* args;
int verbose;

/*****************************************************************************/

int readargs()
{
args->addoption("help", "help", false, "this output", "");
args->addoption("verbose", "v", false, "verbose", "");
args->addoption("level", "l", 0, "level of information", "level");
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
return args->interpret(1);
}

/*****************************************************************************/

int list_of_properties(int level)
{
if (verbose) cout << "list_of_properties:" << endl;
try
  {
  C_confirmation* conf;
  ems_u32* list;
  int i, num;
  ems_i32* buffer;

  cc->version_separator('.', Capab_listproc);
  if (args->numnames()>1)
    {
    int size;
    size=args->numnames()-1;
    list=new ems_u32[size];
    for (i=0, num=0; i<size; i++)
      {
      try
        {
        int id;
        id=cc->procnum(args->getnames(i+1), Capab_listproc);
        list[num++]=id;
        }
      catch (C_error* error)
        {
        cerr << (*error) << endl;
        delete error;
        }
      }
    }
  else
    {
    num=cc->numprocs(Capab_listproc);
    list=new ems_u32[num];
    for (i=0; i<num; i++) list[i]=i;
    }

  conf=cc->GetProcProperties(level, num, list);
  delete list;
  buffer=conf->buffer();
  if ((conf->size()<2) || (buffer[1]!=level) || (buffer[2]!=num))
    {
    cerr << "Formatfehler in \"ProcProperties\"." << endl;
    }
  else
    {
    ems_u32 *ptr;

    ptr=reinterpret_cast<ems_u32*>(buffer)+3;
    for (i=0; i<num; i++)
      {
      char* s;
      int id;

      id=*ptr++;
      cout.setf(ios::left, ios::adjustfield);
      cout << setw(20) << cc->procname(id, Capab_listproc);
      switch (*ptr++)
        {
        case -1: cout << " unknown  "; break;
        case 0 : cout << " constant "; break;
        case 1 : cout << " variable "; break;
        default: cout << " ???????? "; break;
        }
      cout.setf(ios::right, ios::adjustfield);
      if (*reinterpret_cast<ems_i32*>(ptr)==-1)
        cout << "unknown" << endl;
      else
        cout << setw(6) << *ptr << endl;
      ptr++;
      if (level>0)
        {
        ptr=xdrstrdup(s, ptr);
        if (s[0]!=0) cout << "    " << s << endl;
        delete[] s;
        }
      if (level>1)
        {
        ptr=xdrstrdup(s, ptr);
        if (s[0]!=0) cout << "    " << s << endl;
        delete[] s;
        }
      }
    }
  delete conf;
  }
catch(C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  return(-1);
  }
return(0);
}

/*****************************************************************************/
int
main(int argc, char* argv[])
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
catch(C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  return(0);
  }
is=cc->is0();
if (is==0)
  {
  cerr<<"VED "<<cc->name()<<" has no procedures."<<endl;
  }
else
  {
  //cerr << args->numnames()<< "names" << endl;
  list_of_properties(args->getintopt("level"));
  }
delete cc;
communication.done();
}

/*****************************************************************************/
/*****************************************************************************/
