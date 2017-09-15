/*
 * ems_perfd.cc
 * created 28.10.2000 PW
 * 29.10.2000 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <readargs.hxx>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <conststrings.h>
#include <errorcodes.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <inbuf.hxx>
#include <outbuf.hxx>
#include "client_defaults.hxx"

#include <versions.hxx>

VERSION("Mar 27 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: ems_perfd.cc,v 2.2 2004/11/26 22:20:37 wuestner Exp $")
#define XVERSION

C_readargs* args;
int verbose, debug;

int numveds;
struct vedinfo
  {
  C_VED* ved;
  C_instr_system* is;
  C_proclist* proclist;
  bool active;
  };
vedinfo* vedinfos;

int timercounter;
int oldtimercounter;

int command_fd;

/*****************************************************************************/
void inset2(ostream& os) {os<<" ved0 [ved1 ...]";}
void inset4(ostream& os) {os<<"  ved0, ved1 ...    : names of VEDs to be monitored"<<endl;}
void inset7(ostream& os)
{
os<<"  VEDs:";
for (int i=0; i<args->numnames(); i++)
  os<<" "<<args->getnames(i);
os<<endl;
}
/*****************************************************************************/
int readargs()
{
int res;
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("hostname", "h", "localhost",
    "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK,
    "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK,
    "socket for connection with commu", "socket");
args->use_env("sockname", "EMSSOCKET");
args->addoption("interval", "interval", 10, "update every x seconds",
    "interval");
args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
args->hints(C_readargs::exclusive, "port", "sockname", 0);

args->helpinset(2, inset2);
args->helpinset(4, inset4);
args->helpinset(7, inset7);

if ((res=args->interpret(1))!=0) return res;

debug=args->getboolopt("debug");
verbose=debug || args->getboolopt("verbose");

numveds=args->numnames();
if (!numveds)
  {
  cerr<<"no VEDs given"<<endl;
  }
return 0;
}
/*****************************************************************************/
int open_veds()
{
try
  {
  if (debug) cout<<"try communication.init"<<endl;
  if (!args->isdefault("hostname") || !args->isdefault("port"))
    {
    if (debug) cout << "use host \"" << args->getstringopt("hostname")
        << "\", port " << args->getintopt("port") << endl;
    communication.init(args->getstringopt("hostname"),
        args->getintopt("port"));
    }
  else
    {
    if (debug) cout << "use socket \""
        << args->getstringopt("sockname") << "\"" << endl;
    communication.init(args->getstringopt("sockname"));
    }
  if (debug) cout<<"try open ved"<<endl;
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(-1);
  }
try
  {
  vedinfos=new vedinfo[numveds];
  if (!vedinfos)
    {
    cerr<<"new vedinfo["<<numveds<<"]: "<<strerror(errno)<<endl;
    communication.done();
    return(-1);
    }
  for (int i=0; i<numveds; i++)
    {
    vedinfos[i].ved=new C_VED(args->getnames(i));
    vedinfos[i].ved->confmode(asynchron);
    vedinfos[i].is=vedinfos[i].ved->is0();
    vedinfos[i].proclist=vedinfos[i].is->newproclist();
    vedinfos[i].proclist->add_proc("Timestamp");
    vedinfos[i].proclist->add_par(1);
    vedinfos[i].proclist->add_proc("GetSyncStatist");
    vedinfos[i].proclist->add_par(4, /*hists*/7, 0, /*maxchan*/100, 0);
    vedinfos[i].active=true;
    }
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  communication.done();
  return(-1);
  }
if (verbose)
  cout<<"veds open via "<<communication<<endl;
return(0);
}
/*****************************************************************************/
void request_data()
{
for (int i=0; i<numveds; i++)
  {
  if (vedinfos[i].active)
    {
    try
      {
      vedinfos[i].is->command(vedinfos[i].proclist);
      }
    catch (C_error* e)
      {
      cerr << (*e) << endl;
      delete e;
      }
    }
  }
}
/*****************************************************************************/
void read_data()
{
C_confirmation* conf=communication.GetConf();
}
/*****************************************************************************/
void read_command()
{
}
/*****************************************************************************/
void mainloop()
{
fd_set readfds;
/*fd_set writefds;*/
int res;
int n;

while (1)
  {
  n=0;
  FD_ZERO(&readfds);
  if (communication.valid())
    {
    FD_SET(communication.path(), &readfds);
    if (communication.path()>n) n=communication.path();
    }
  if (command_fd>=0)
    {
    FD_SET(command_fd, &readfds);
    if (command_fd>n) n=command_fd;
    }

  res=select(n+1, &readfds, 0, 0, 0);
  if (res<0)
    {
    if (errno!=EINTR)
      {
      cerr<<"select: "<<strerror(errno)<<endl;
      }
    res=0;
    }
  else if (res>0)
    {
    if ((communication.path()>=0) && FD_ISSET(communication.path(), &readfds))
      {
      read_data();
      }
      if ((command_fd>=0) && FD_ISSET(command_fd, &readfds))
      {
      read_command();
      }
    }
  if (timercounter!=oldtimercounter)
    {
    request_data();
    oldtimercounter=timercounter;
    }
  }
}
/*****************************************************************************/
void alarm_handler(int sig)
{
timercounter++;
}
/*****************************************************************************/
main(int argc, char* argv[])
{
try
  {
  struct sigaction action;
  
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return 1;

  if (open_veds()<0) return 1;

  timercounter=oldtimercounter=0;
  action.sa_handler=alarm_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags=0;
  if (sigaction(SIGALRM, &action, 0)<0)
    {
    cerr<<"sigaction(SIGALRM, ...): "<<strerror(errno)<<endl;
    communication.done();
    return 1;
    }
  alarm(args->getintopt("interval"));
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }

command_fd=0;
mainloop();

communication.done();
}
/*****************************************************************************/
/*****************************************************************************/
