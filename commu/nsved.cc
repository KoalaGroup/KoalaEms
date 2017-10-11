/*
 * nsved.cc
 * 
 * created: 17.07.95 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <commu_def.hxx>
#include <signal.h>
#include <readargs.hxx>
#include <errors.hxx>
#include <ems_errors.hxx>
#include <errno.h>
#include <ved_addr.hxx>
#include <xferbuf.hxx>
#include <commu_zdb_new.hxx>
#include <commu_nonedb.hxx>
#include <commu_ns.hxx>
#include <gnuthrow.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: nsved.cc,v 2.10 2014/07/14 15:12:20 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_readargs* args;
int verbose;
int debug;
int running;
C_db* db=0;

/*****************************************************************************/

int readargs()
{
args->addoption("verbose", "v", C_readargs::false, "verbose");
args->addoption("debug", "d", C_readargs::false, "debug");
args->addoption("commlist", "commlist", DEFCOMMBEZ,
    "communication relations list", "commlist");
args->use_env("commlist", "COMMLIST");
return(args->interpret(1));
}

/*****************************************************************************/

C_nsconf* do_request(const C_nsreq& req)
{
static int i=0;
switch (req.type())
  {
  case ns_none:
    return(new C_nsconf_none);
    break;
  case ns_exit:
    cerr << "ns: exit" << endl;
    running=0;
    return(new C_nsconf_exit);
    break;
  case ns_dbaddr:
    cerr << "ns: dbaddr" << endl;
    return(new C_nsconf_dbaddr(db->listname()));
    break;
  case ns_new_dbaddr:
    cerr << "ns: new_dbaddr" << endl;
    delete db;
    try
      {
      db=new C_zdb(((C_nsreq_new_dbaddr*)&req)->name());
      }
    catch (C_error* e)
      {
      db=new C_nonedb;
      return(new C_nsconf_error(e));
      }
    catch (...)
      {
      cerr << "caught ... in do_request() in nsved.cc" << endl;
      exit;
      }
    return(new C_nsconf_new_dbaddr);
    break;
  case ns_vedaddr:
    {
    if (i++%2)
      {
      C_VED_addr* addr=new C_VED_addr_inet("v08", 2048);
      return(new C_nsconf_vedaddr(addr));
      }
    else
      {
      return(new C_nsconf_error(new C_ems_error(EMSE_UnknownVED,
          "nsved: vedaddr")));
      }
    }
    break;
  case ns_new_vedaddr:
    return(new C_nsconf_error(new C_program_error("not implemented")));
    break;
  case ns_vedlist:
    return(new C_nsconf_error(new C_program_error("not implemented")));
    break;
  default:
    return(new C_nsconf_error(new C_program_error("unknown request")));
    break;
  }
}

/*****************************************************************************/

void sig_int(int sig)
{
cerr << "signal " << sig << " occured" << endl;
}

/*****************************************************************************/

main(int argc, char* argv[])
{
C_sender* sender;
C_receiver* receiver;

signal(SIGINT, sig_int);
signal(SIGTERM, sig_int);

try
  {
  args=new C_readargs(argc, argv);
  if (readargs()!=0) return(0);

  debug=args->getboolopt("debug");
  verbose=debug || args->getboolopt("verbose");

  sender=new C_sender(3);
  receiver=new C_receiver(3);
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  return(0);
  }
catch (...)
  {
  cerr << "caught ... in main() in nsved.cc (1)" << endl;
  exit;
  }

try
  {
  db=new C_zdb(args->getstringopt("commlist"));
  }
catch (C_error* e)
  {
  try
    {
    C_outbuf outbuf;
    outbuf << (*e);
    delete e;
    sender->send(outbuf);
    }
  catch(C_error* e)
    {
    clog << "nsved; send: "<< (*e) << endl;
    delete e;
    }
  catch (...)
    {
    cerr << "caught ... in main() in nsved.cc (2)" << endl;
    exit;
    }
  goto raus;
  }
catch (...)
  {
  cerr << "caught ... in main() in nsved.cc (3)" << endl;
  exit;
  }

try
  {
  C_outbuf outbuf;
  outbuf << C_none_error();
  sender->send(outbuf);
  }
catch (C_error* e)
  {
  clog << "nsved; send: "<< (*e) << endl;
  delete e;
  goto raus;
  }
catch (...)
  {
  cerr << "caught ... in main() in nsved.cc (4)" << endl;
  exit;
  }

C_inbuf* inbuf;
running=1;
while (running)
  {
  try
    {
    inbuf=receiver->receive();
    clog << "ns: &inbuf=" << (void*)inbuf << endl;
    C_nsreq* req=extract_nsreq(*inbuf);
    delete inbuf;

    C_nsconf* conf=do_request(*req);

    C_outbuf outbuf;
    outbuf << (*conf);
    sender->send(outbuf);
    }
  catch (C_error* e)
    {
    clog << "nsved; loop: "<< (*e) << endl;
    delete e;
    running=0;
    }
  catch (...)
    {
    cerr << "caught ... in main() in nsved.cc (5)" << endl;
    exit;
    }
  }

cerr << "nsved: hinter mainloop" << endl;
raus:
sleep(5);
delete sender;
delete receiver;

return(0);
}

/*****************************************************************************/
/*****************************************************************************/
