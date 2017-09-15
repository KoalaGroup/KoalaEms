/*
 * vedinfo.cc                       
 * created: 23.11.94 PW
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
#include <errno.h>
#include <sys/ioctl.h>
#include <outbuf.hxx>
#include <inbuf.hxx>
#include <ved_addr.hxx>
#include <client_defaults.hxx>
#include <versions.hxx>

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: vedinfo.cc,v 2.16 2007/04/18 20:13:34 wuestner Exp $")
#define XVERSION

C_instr_system* is;
C_readargs* args;
int verbose;

/*****************************************************************************/

int readargs()
{
args->addoption("help", "help", false, "this output");
args->addoption("all", "a", false, "all infos");
args->addoption("verbose", "v", false, "verbose");
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
args->addoption("open", "o", false, "perform open");
return args->interpret(1);
}

/*****************************************************************************/

#ifdef __GNUC__
STRING* newString(int n)
{
return new STRING[n];
}
#endif

/*****************************************************************************/

int vedlist(STRING** list)
{
int num=0;

if (verbose) cout << "get all open VEDs" << endl;
try
  {
  C_confirmation* conf;
  int i;

  conf=is->command("VEDinfo");
  C_inbuf inbuf(conf->buffer(), conf->size());
  inbuf++;
  inbuf >> num;
#ifndef __GNUC__
  *list=new STRING[num];
#else
  *list=newString(num);
#endif
  for (i=0; i<num; i++) inbuf >> (*list)[i];
  delete conf;
  }
catch(C_error* error)
  {
  cerr << (*error) << endl;
  delete error;
  }
return(num);
}

/*****************************************************************************/

int ved_info()
{
STRING* list=0;
int num;

if (verbose) cout << "ved_info" << endl;
num=vedlist(&list);
if (isatty(1))
  {
  int i;
  int max, n;
  struct winsize ttysize;
  int width, columns;

  max=0;
  for (i=0; i<num; i++) if ((n=list[i].length())>max) max=n;
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
  }
else
  {
  int i;
  for (i=0; i<num; i++) cout << list[i] << endl;
  }

delete[] list;
return(0);
}

/*****************************************************************************/

int ved_all_info()
{
STRING* list=0;
int num;
C_VED* ved;
int n;

if (verbose) cout << "ved_all_info" << endl;
if ((num=args->numnames())>0)
  {
  int i;
  list=new STRING[num];
  for (i=0; i<num; i++) list[i]=args->getnames(i);
  }
else
  num=vedlist(&list);

for (n=0; n<num; n++)
  {
  try
    {
    C_confirmation* conf;
    int status;

    cout << list[n] << ": " << flush;
    if (args->getboolopt("open"))
      ved=new C_VED(list[n]);
    else
      ved=0;

#ifdef USE_STRING_H
    conf=is->command("VEDinfo", list[n]);
#else
    conf=is->command("VEDinfo", list[n].c_str());
#endif
    C_inbuf inbuf(conf->buffer(), conf->size());
    inbuf++;
    cout << inbuf.getint() << endl;
    cout << "  status: ";
    inbuf >> status;
    switch (status)
      {
      case 0: cout << "non_existent"; break;
      case 1: cout << "connecting"; break;
      case 2: cout << "opening"; break;
      case 3: cout << "read_ident"; break;
      case 4: cout << "error_ident"; break;
      case 5: cout << "ready"; break;
      case 6: cout << "disconnecting"; break;
      case 7: cout << "close"; break;
      default: cout << status << endl;
      }
    cout << endl;
    if (status==5)
      {
      C_VED_addr* addr;
      int i, num;

      cout << "  VED-Ver=" << inbuf.getint() << "; Reqtab-Ver="
          << inbuf.getint()
          << "; Unsoltab-Ver=" << inbuf.getint() << endl;
      addr=extract_ved_addr(inbuf);
      cout << "  address: " << addr << endl;
/*
      switch (addr_typ)
        {
        case 0: cout << "notvalid" << endl; break;
        case 1: cout << "builtin" << endl; break;
        case 2:
          {
          char* s;
          ptr=xdrstrdup(s, ptr);
          cout << "unix; " << s << endl;
          delete[] s;
          }
          break;
        case 3:
          {
          char* s;
          ptr=xdrstrdup(s, ptr);
          cout << "inet; " << s << " " << *ptr++ << endl;
          delete[] s;
          }
          break;
        case 4:
          {
          char* s;
          int num, i;
          ptr=xdrstrdup(s, ptr);
          cout << "inet_path; " << s << " " << *ptr++;
          delete[] s;
          num=*ptr++;
          for (i=0; i<num; i++)
            {
            ptr=xdrstrdup(s, ptr);
            cout << " " << s;
            delete[] s;
            }
          cout << endl;
          }
          break;
        case 5: cout << "vic" << endl; break;
        default: cout << "??? (" << addr_typ << ')' << endl;
        }
*/
      inbuf >> num;
      cout << "  " << num << (num==1?" client:":" clients:");
      for (i=0; i<num; i++)
        {
        STRING s;
        inbuf >> s;
        cout << " " << s;
        }
      cout << endl;
      inbuf >> num;
      cout << "  " << num << (num==1?" unsol. client:":" unsol. clients:");
      for (i=0; i<num; i++) cout << " " << inbuf.getint();
      cout << endl;
      // hier geht's weiter

      delete addr;
      }

    cout << endl;
    delete conf;
    if (ved) delete ved;
    }
  catch(C_error* error)
    {
    cerr << endl << "  " << (*error) << endl;
    delete error;
    }
  }

delete[] list;
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
  if ((args->numnames()>0) || (args->getboolopt("all")))
    ved_all_info();
  else
    ved_info();
  }
delete cc;
communication.done();
}

/*****************************************************************************/
/*****************************************************************************/
