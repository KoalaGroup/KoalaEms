/*
 * emswatch.cc  
 * created: 20.01.95 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <readargs.hxx>
#include <errno.h>
#include <sys/ioctl.h>
#include <proc_communicator.hxx>
#include <proc_is.hxx>
#include <msg.h>
#include <ved_errors.hxx>
#include <errors.hxx>
#include <ems_message.hxx>
#include <compat.h>
#include <client_defaults.hxx>
#include <clientcomm.h>
#include <versions.hxx>

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: emswatch.cc,v 2.11 2007/04/18 19:42:15 wuestner Exp $")
#define XVERSION

C_readargs* args;
C_VED *commu_i, *commu_l;
C_instr_system *is_i, *is_l;
int verbose, debug, logtype;
ofstream* ofile;
int running;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "v", false, "verbose");
args->addoption("debug", "d", false, "debug");
args->addoption("logtype", "logtype", 1, "logtype");
args->addoption("hostname", "h", "localhost", "host running commu", "hostname");
args->use_env("hostname", "EMSHOST");
args->addoption("port", "p", DEFISOCK, "port for connection with commu", "port");
args->use_env("port", "EMSPORT");
args->addoption("sockname", "s", DEFUSOCK, "socket for connection with commu",
    "socket");
args->use_env("sockname", "EMSSOCKET");
args->hints(C_readargs::exclusive, "hostname", "sockname", 0);
args->hints(C_readargs::exclusive, "port", "sockname", 0);
return args->interpret(1);
}

/*****************************************************************************/

void msgloop()
{
cout << "watching....." << endl;
running=1;
while (running)
  {
  C_confirmation* conf;
  try
    {
    do
      {
      conf=communication.GetConf();
      //if (conf==0) cerr << "null confirmation!!" << endl;
      } while (conf==0);
    C_EMS_message msg(conf, 0, 0);
    delete conf;
    msg.print();
    }
  catch (C_error* e)
    {
    cout << endl << (*e) << endl;
    cerr << endl << (*e) << endl;
    delete e;
    running=0;
    cout << endl << "rausgeflogen!" << endl << endl << endl;
    }
  }
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);
policies=static_cast<en_policies>(pol_nodebug|pol_nocount);
try
  {
  debug=args->getboolopt("debug");
  verbose=debug || args->getboolopt("verbose");

  logtype=args->getintopt("logtype");
  if (!args->isdefault("hostname") || !args->isdefault("port"))
    communication.init(args->getstringopt("hostname"),
        args->getintopt("port"));
  else
    communication.init(args->getstringopt("sockname"));

  commu_i=new C_VED("commu");
  is_i=commu_i->is0();
  ved_versions=new C_VED_versions(is_i);
  commu_l=new C_VED("commu_l");
  is_l=commu_l->is0();
  C_confirmation* conf;
  conf=is_l->command("Add_all", 1, logtype);
  delete conf;
  msgloop();
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
