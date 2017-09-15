/*
 * clientinfo.cc  
 * created: 25.11.94 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <string.h>
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
#include "client_defaults.hxx"
#include <clientcomm.h>
#include <versions.hxx>

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: clientinfo.cc,v 2.12 2009/10/02 11:21:12 wuestner Exp $")
#define XVERSION

C_instr_system* is;
C_readargs* args;
int verbose;

/*****************************************************************************/

int readargs()
{
args->addoption("help", "help", false, "this output", "");
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
args->addoption("vedname", "ved", "commu", "Name des VED", "vedname");
return args->interpret(1);
}

/*****************************************************************************/

int clientlist(char*** list)
{
*list=0;
int num=0;

if (verbose) cout << "get all clients" << endl;
try
  {
  C_confirmation* conf;

  conf=is->command("Clientinfo");
  if (conf->size()<2)
    {
    cerr << "Formatfehler in \"clientinfo\"." << endl;
    }
  else
    {
    ems_u32* ptr;
    int i;
    num=conf->buffer(1);
    *list=new char*[num];
    for (i=0, ptr=reinterpret_cast<ems_u32*>(conf->buffer())+2; i<num; i++)
        ptr=xdrstrdup((*list)[i], ptr);
    }
  delete conf;
  }
catch(C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
return(num);
}

/*****************************************************************************/

int client_info()
{
char** list;
int num;

if (verbose) cout << "client_info" << endl;
num=clientlist(&list);
if (isatty(1))
  {
  int i;
  int max, n;
  struct winsize ttysize;
  int width, columns;

  max=0;
  for (i=0; i<num; i++) if ((n=strlen(list[i]))>max) max=n;
  if (ioctl(0, TIOCGWINSZ, reinterpret_cast<char*>(&ttysize))==0)
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
  for (i=0; i<num; i++)
    {
    cout << setw(max) << list[i] << "  ";
    if (((i+1)%columns==0) || (i+1==num)) cout << endl;
    }
  delete list;
  }
else
  {
  int i;
  for (i=0; i<num; i++) cout << list[i] << endl;
  }

if (list)
  {
  int i;
  for (i=0; i<num; i++) delete list[i];
  delete list;
  }

return(0);
}

/*****************************************************************************/

int client_all_info()
{
char** list;
int num, names;
int n;

if (verbose) cout << "client_all_info" << endl;
if ((num=args->numnames())>0)
  {
  int i;
  names=1;
  list=new char*[num];
  for (i=0; i<num; i++) list[i]=const_cast<char*>(args->getnames(i));
  }
else
  {
  names=0;
  num=clientlist(&list);
  }

for (n=0; n<num; n++)
  {
  try
    {
    C_confirmation* conf;
    int* buffer;

    cout << list[n] << ": " << flush;
    conf=is->command("Clientinfo", list[n]);
    buffer=conf->buffer()+1;
    cout << static_cast<unsigned int>(buffer[0]) << endl;
    cout << "  status: ";
    switch (buffer[1])
      {
      case 0: cout << "non_existent"; break;
      case 1: cout << "connecting"; break;
      case 2: cout << "opening"; break;
      case 3: cout << "read_ident"; break;
      case 4: cout << "error_ident"; break;
      case 5: cout << "ready"; break;
      case 6: cout << "disconnecting"; break;
      case 7: cout << "close"; break;
      default: cout << buffer[1] << endl;
      }
    cout << endl;
    if (buffer[1]==5)
      {
      ems_u32* ptr;
      int i, num;
//      cout << "  remote: " << buffer[2] << endl;
      ptr=reinterpret_cast<ems_u32*>(buffer)+3;
      if (buffer[2])
        {
        char* s;
        int addr;

        cout << "  connection: " << (*ptr++?"tcp":"unix") << endl;
        ptr=xdrstrdup(s, ptr);
        addr=*ptr++;
        cout << "  host: " << s << " ("
          << ((addr>>24)&0xff) << '.'
          << ((addr>>16)&0xff) << '.'
          << ((addr>> 8)&0xff) << '.'
          << (addr&0xff) << ')' << endl;
        delete s;
        ptr=xdrstrdup(s, ptr);
        cout << "  user: " << s << "(" << *ptr++;
        cout << '.' << *ptr++ << ')' << endl;
        delete s;
        ptr=xdrstrdup(s, ptr);
        cout << "  experiment: " << s << endl;
        delete s;
//        cout << "  bigend: " << *ptr++ << endl;
        ptr++;
        }
      en_policies policies=static_cast<en_policies>(*ptr++);
      if (policies&pol_nodebug) cout<<"policy nodebug"<<endl;
      if (policies&pol_noshow) cout<<"policy noshow"<<endl;
      if (policies&pol_nocount) cout<<"policy nocount"<<endl;
      if (policies&pol_nowait) cout<<"policy nowait"<<endl;
      num=*ptr++;
      cout << "  " << num << " server:";
      for (i=0; i<num; i++)
        {
        char* s;
        ptr=xdrstrdup(s, ptr);
        cout << " " << s;
        delete s;
        }
      cout << endl;
      // hier geht's weiter
      }

    cout << endl;
    delete conf;
    }
  catch(C_error* e)
    {
    cerr << endl << "  " << (*e) << endl;
    delete e;
    }
  }

if (list)
  {
  if (!names) {int i; for (i=0; i<num; i++) delete list[i];}
  delete list;
  }
return(0);
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
C_VED* cc;
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
  cc=new C_VED(args->getstringopt("vedname"));
  if (verbose) cout << "ok." << endl;
  }
catch(C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(0);
  }

is=cc->is0();
if (is==0)
  {
  cerr<<"VED "<<cc->name()<<" has no procedures."<<endl;
  }
else
  {
  if ((args->numnames()>0) || (args->getboolopt("all")))
    client_all_info();
  else
    client_info();
  }
delete cc;
communication.done();
}

/*****************************************************************************/
/*****************************************************************************/
