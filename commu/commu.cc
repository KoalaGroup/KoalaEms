/*
 * commu.cc
 * 
 * created before 07.02.94
 * 
 */

#define _main_c_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
//#define _ANSI_C_SOURCE
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <stdlib.h>
//#define _XOPEN_SOURCE_EXTENDED
#include <unistd.h>
//#undef _XOPEN_SOURCE_EXTENDED
#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#include <string.h>
#endif
#include <signal.h>

#include <debug.hxx>
#include <readargs.hxx>
#include <commu_def.hxx>
#include <commu_logg.hxx>
#include <commu_log.hxx>
#include <commu_sock.hxx>
#include <commu_server.hxx>
#include <commu_io_server.hxx>
#include <commu_client.hxx>
#include <commu_io_client.hxx>
#include <commu_db.hxx>
#include <commu_zdb.hxx>
#ifdef NSDB
#include <commu_nsdb.hxx>
#endif
#ifdef COSYDB
#include <commu_cdb.hxx>
#endif
#ifdef ODB
#include <commu_odb.hxx>
#endif
//#include <commu_anno.hxx>
#include <daemon.hxx>
#include <errors.hxx>
#include <linkdate.hxx>
#include <cppdefines.hxx>
#include <compat.h>

#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu.cc,v 2.37 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

#define __xSTRING(x) #x
#define __SSTRING(x) __xSTRING(x)

using namespace std;

/*****************************************************************************/

int autoexit=0;
int allowautoexit=1;
int leavemainloop=0;
int leaveimmediatly=0;
int termsig=0;
string grund="";
int background;
int dtablesize;
loggstruct lgg;
C_elog elog(lgg); // errors
C_nlog nlog(lgg); // notifications
C_dlog dlog(lgg); // debugmessages
C_db *cdb=0;      // database
C_unix_socket mainusock;
C_tcp_socket mainisock;
int maindsock=-1;

const char *sockname;
short port;

C_clientlist clientlist(20, "clientlist");
C_serverlist serverlist(20, "serverlist");
C_ints globalunsollist(10, "global_unsollist");

const char *commlist;

#ifdef COSYDB
int use_cosydb;
#endif
#ifdef COSYLOG
int use_cosylog;
C_cosylogger* lgg_col=0;
#endif

#ifdef ODB
const char *dbname;
const char *fdbname;
int use_odb;
#endif
#ifdef NSDB
int use_nsdb;
#endif

C_readargs* args;

/*****************************************************************************/

void printoptions()
{
cout << "linked at " << linkdate << endl;

for (int i=0; cppdefs[i]!=0; i++) cout << cppdefs[i] << endl;
}

/*****************************************************************************/

int readargs()
{
args->addoption("query", "q", false, "query for compile options");

args->addoption("socket", "s", __SSTRING(DEFUSOCKET),
    "socket for local communication", "socket");
args->use_env("socket", "EMSSOCKET");

args->addoption("port", "p", DEFISOCKET,
    "socket for internet communication (TCP/IP)", "port");
args->use_env("port", "EMSPORT");
args->addoption("nosearch", "nosearch", true, "don't search for free port");
args->addoption("noinet", "noinet", false, "don't open ip socket");
args->addoption("forkwatcher", "forkwatcher", "", "fork watch process", "watch");
args->addoption("watchfile", "watchfile", "commuwatch", "logfile for watcher", "watchlog");
args->use_env("forkwatcher", "COMMUWATCH");
args->use_env("watchfile", "COMMUWATCHLOG");
#ifdef COSYDB
args->addoption("commlist", "l", __SSTRING(DEFCOMMBEZ),
    "communication relations list; overrides 'c'", "commlist");
#else
args->addoption("commlist", "l", __SSTRING(DEFCOMMBEZ),
    "communication relations list", "commlist");
#endif
args->use_env("commlist", "COMMLIST");

args->addoption("logfile", "logfile", "", "logfile");
args->multiplicity("logfile", 0);

args->addoption("pidfile", "pidfile", "/var/run/commu.pid", "pidfile");

args->addoption("nobackground", "nb", false,
    "don't put program into background");

#ifdef COSYDB
#ifdef COSYLOG
args->addoption("cosydb", "c", false,
    "use COSY database in shared memory and COSY logging");
#else
args->addoption("cosydb", "c", false,
    "use COSY database in shared memory");
#endif // COSYLOG
#endif // COSYDB

#ifdef COSYLOG
#ifdef COSYDB
args->addoption("cosylog", "cl", false,
    "use COSY logging; default if c is specified");

args->addoption("nocosylog", "ncl", false, "don't use COSY logging");
#else
args->addoption("cosylog", "cl", false, "use COSY logging");
#endif // COSYDB
#endif // COSYLOG

#ifdef ODB
args->addoption("database", "db", "", "database", "database");

args->addoption("fdatabase", "fdb", "",
  "federated database", "federated database");
#endif // ODB

#ifdef DEBUG
args->addoption("debug", "D", false, "debug");
args->use_env("debug", "EMSDEBUG");
args->addoption("verbose", "v", false, "verbose debugging");
args->use_env("verbose", "EMSVERBOSE");
#ifdef TRACE
args->addoption("trace", "t", false, "procedure trace");
#endif // TRACE
#endif // DEBUG

#ifdef NSDB
args->addoption("ns", "ns", false, "use nameserver");
args->addoption("nsprog", "nsprog", "nsved", "nameserver program",
    "nsprog");
args->addoption("nshost", "nshost", "localhost", "host for VED-nameserver",
    "nshost");
args->addoption("nsport", "nsport", 12345, "port for VED-nameserver",
    "nshost");

args->hints(C_readargs::implicit, "nsprog", "ns", 0);
args->hints(C_readargs::implicit, "nshost", "ns", 0);
args->hints(C_readargs::implicit, "nsport", "ns", 0);
#endif // NSDB

#if defined(COSYDB) && defined(NSDB)
args->hints(C_readargs::exclusive, "ns", "cosydb", 0);
#endif
args->helpinset(0, printlinkdate);
if (args->interpret(1)!=0) return(-1);

if (args->getboolopt("query"))
  {
  printoptions();
  return(-1);
  }

// immer
#ifdef COSYDB
use_cosydb=0;
#endif
#ifdef ODB
use_odb=0;
#endif
commlist=args->getstringopt("commlist");
if (strcmp(commlist, ".")==0) commlist=__SSTRING(DEFCOMMBEZ);
sockname=args->getstringopt("socket");
port=args->getintopt("port");
background=!args->getboolopt("nobackground");

#ifdef DEBUG
debug=0;
if (args->getboolopt("debug")) debug|=D_TR;
if (args->getboolopt("verbose")) debug|=D_DUMP;
#ifdef TRACE
if (args->getboolopt("trace")) debug|=D_TRACE;
#endif // TRACE
#endif // DEBUG

#ifdef COSYDB
use_cosydb=args->getboolopt("cosydb") && !args->isargument("commlist");
#endif // COSYDB

#ifdef COSYLOG
#ifdef COSYDB
if (use_cosydb)
  use_cosylog=!args->getboolopt("nocosylog");
else
  use_cosylog=args->getboolopt("cosylog");
#else
use_cosylog=args->getboolopt("cosylog");
#endif // COSYDB
#endif // COSYLOG

#ifdef ODB
dbname=args->getstringopt("database");
fdbname=args->getstringopt("fdatabase");
#ifdef COSYDB
use_odb=(!use_cosydb && !args->isdefault("database"));
#else
use_odb=(!args->isdefault("database"));
#endif
#endif

#ifdef NSDB
use_nsdb=!args->isdefault("ns");
#endif

return(0);
}

/*****************************************************************************/

void forkwatcher()
{
if (args->isdefault("forkwatcher")) return;
const char* watch=args->getstringopt("forkwatcher");
const char* watchlog=args->getstringopt("watchfile");
nlog << "watcher  : " << watch << flush;
nlog << "watchfile: " << watchlog << flush;
int cpid;
if ((cpid=fork())==-1)
  {
  elog << "not forked: " << strerror(errno) << flush;
  return;
  }
if (cpid==0)
  {
  execlp((char*)watch, (char*)watch, "-logtype 2", "-logfile", watchlog,
      "-s", sockname, (char*)0);
  clog << "not execed: " << strerror(errno) << endl;
  _exit(0);
  }
nlog << "watcher pid: " << cpid << flush;
}

/*****************************************************************************/
void acceptmainsocket(int sock)
{/* acceptmainsocket */
    C_socket *ns;
#if 0
    C_io_client *client;
#endif

    TR(acceptmainsocket)
    try {
        if (sock==(int)mainusock) {
            ns=mainusock.accept();
        } else if (sock==(int)mainisock) {
            ns=mainisock.accept();
#if 0
        } else if (sock==(int)maindsock) {
    }
#endif
        } else {
            elog << "Programmfehler! unbekannter Mainsocket " << sock << flush;
            ns=NULL;
        }
    } catch(C_error* e) {
        elog << (*e) << flush;
        delete e;
        ns=NULL;
    }
    if (ns!=NULL) {
#if 0
        client=new C_io_client(ns);
#else
        /* the return value of new is not used,
           C_io_client registers itself somewhere */
        new C_io_client(ns);
#endif
    }
}/* acceptmainsocket */
/*****************************************************************************/

int openusocket()
{
C_unix_socket usock;
int ok;

TR(openusocket)
usock.setname(sockname);
ok=0;
try
  {
  usock.connect();
  }
catch(C_unix_error* e)
  {
  delete e;
  ok=1;
  }
catch(C_error* e)
  {
  elog << "test connection: " << (*e) << flush;
  delete e;
  ok=1;
  }
if (!ok)
  {
  usock.close();
  nlog << time << "socket " << sockname << " already used" << flush;
  return(-1);
  }

unlink((char*)sockname);
mainusock.setname(sockname);
if (mainusock.bind()==-1)
  {
  elog << "mainusock.bind(" << sockname << ")" << error << flush;
  return(-1);
  }
if (mainusock.listen(8)==-1)
  {
  elog << "mainusock.listen(" << sockname << ")" << error << flush;
  return(-1);
  }
return(0);
}

/*****************************************************************************/

int openisocket()
{
int res;

TR(openisocket)

if (args->getboolopt("noinet"))
  {
  mainisock.close();
  return 0;
  }

if (args->getboolopt("nosearch"))
  {
  mainisock.setname(string(""), port);
  if ((res=mainisock.bind())==-1)
    {
    elog << "mainisock.bind(" << port << ")" << error << flush;
    return -1;
    }
  }
else
  {
  do
    {
    mainisock.setname(string(""), port);
    if ((res=mainisock.bind())==-1)
      {
      if (errno==EADDRINUSE) port++;
      }
    }
  while ((res!=0) && !((res==-1) && (errno!=EADDRINUSE)));
  }
if (mainisock.listen(8)==-1)
  {
  elog << "mainisock.listen(" << port << ")" << error << flush;
  return(-1);
  }
return(0);
}

/*****************************************************************************/

void dowrite(fd_set *fds, int maxpath)
{
int i;
C_io_server* server;
C_io_client* client;

TR(dowrite)
for (i=0; i<=maxpath; i++)
  if (FD_ISSET(i, fds))
    {
    if ((server=(C_io_server*)serverlist.findsock(i))!=0)
      {
      server->write_sel();
      }
    else if ((client=(C_io_client*)clientlist.findsock(i))!=0)
      {
      client->write_sel();
      }
    else
      {
      elog << "dowrite: Fehler: pfad " << i <<" hat keinen Eigentuemer."
          << flush;
      }
    }
}

/*****************************************************************************/

void doread(fd_set *fds, int maxpath)
{
int i;
C_io_server* server;
C_io_client* client;

TR(doread)
for (i=0; i<=maxpath; i++)
  if (FD_ISSET(i, fds))
    {
    if ((i!=(int)mainusock) && (i!=(int)mainisock) && (i!=(int)maindsock))
      {
      if ((server=(C_io_server*)serverlist.findsock(i))!=0)
        {
        server->read_sel();
        }
      else if ((client=(C_io_client*)clientlist.findsock(i))!=0)
        {
        client->read_sel();
        }
      else
        {
        elog << "doread: Fehler: pfad " << i << " hat keinen Eigentuemer."
            << flush;
        }
      }
    }
}

/*****************************************************************************/

void doexcept(fd_set *fds, int maxpath)
{
int i;
C_io_server* server;
C_io_client* client;

TR(dowrite)
for (i=0; i<=maxpath; i++)
  if (FD_ISSET(i, fds))
    {
    if ((server=(C_io_server*)serverlist.findsock(i))!=0)
      {
      server->except_sel();
      }
    else if ((client=(C_io_client*)clientlist.findsock(i))!=0)
      {
      client->except_sel();
      }
    else
      {
      elog << "doexcept: Fehler: pfad " << i <<" hat keinen Eigentuemer."
          << flush;
      }
    }
}

/*****************************************************************************/

void addacceptfds(fd_set *fds, int& maxpath)
{/* addacceptfds */
TR(addacceptfds)
if (mainusock>-1)
  {
  FD_SET(mainusock, fds);
  if (mainusock>maxpath) maxpath=mainusock;
  }
if (mainisock>-1)
  {
  FD_SET(mainisock, fds);
  if (mainisock>maxpath) maxpath=mainisock;
  }
if (maindsock>-1)
  {
  FD_SET(maindsock, fds);
  if (maindsock>maxpath) maxpath=maindsock;
  }
}/* addacceptfds */

/*****************************************************************************/

void makereadfds(fd_set *fds, int& maxpath)
{/* makereadfds */
TR(makereadfds)
FD_ZERO(fds);
serverlist.readfield(fds, maxpath);
clientlist.readfield(fds, maxpath);
}/* makereadfds */

/*****************************************************************************/

void makewritefds(fd_set *fds, int& maxpath)
{/* makewritefds */
TR(makewritefds)
FD_ZERO(fds);
serverlist.writefield(fds, maxpath);
clientlist.writefield(fds, maxpath);
}/* makewritefds */

/*****************************************************************************/

void makeexceptfds(fd_set *fds, int& maxpath)
{
TR(makewritefds)
FD_ZERO(fds);
serverlist.exceptfield(fds, maxpath);
clientlist.exceptfield(fds, maxpath);
}

/*****************************************************************************/

void mainloop()
{/* mainloop */
fd_set readfds, writefds, exceptfds;
register int res, sock;
int size, i;

TR(mainloop)
// regular loop
// executed until leavemainloop is set
//  or autoexit is set and clientlist contains only hidden clients
do
  {
  int maxpath=-1;
  makereadfds(&readfds, maxpath);
  addacceptfds(&readfds, maxpath);
  makewritefds(&writefds, maxpath);
  makeexceptfds(&exceptfds, maxpath);
  res=select(maxpath+1, &readfds, &writefds, &exceptfds, NULL);
  if ((res==-1) && (errno!=EINTR))
    {
    elog << "mainloop:select 1" << error << flush;
    return;
    }
  if (res>0)
    {
    if ((sock=mainusock)>=0)
      if (FD_ISSET(sock, &readfds)) acceptmainsocket(sock);
    if ((sock=mainisock)>=0)
      if (FD_ISSET(sock, &readfds)) acceptmainsocket(sock);
    if ((sock=maindsock)>=0)
      if (FD_ISSET(sock, &readfds)) acceptmainsocket(sock);
    doexcept(&exceptfds, maxpath);
    doread(&readfds, maxpath);
    dowrite(&writefds, maxpath);
    }
  }
while(!leavemainloop && !(autoexit && clientlist.leer()));

elog<<"first loop finished"<<flush;
if (leavemainloop)
  if (leaveimmediatly)
    grund=" (immediate exit)";
  else
    if (termsig==SIGINT)
      grund=" (^C)";
    else if (termsig==SIGTERM)
      grund=" (kill)";
    else
      grund=" (exit)";
else
  grund=" (autoexit)";

leavemainloop=0;

// close sockets to avoid new connections
if ((int)mainusock!=-1) mainusock.close();
if ((int)mainisock!=-1) mainisock.close();
//  if (maindsock!=-1) maindsock.close();

if (leaveimmediatly) return;

// inform all clients
size=clientlist.size();
for (i=0; i<size; i++) clientlist[i]->bye();

// execute loop until clientlist contains only noncounting clients
elog<<"enter second loop"<<flush;
while (!clientlist.leer() && !leavemainloop)
  {
  int maxpath=-1;
  makereadfds(&readfds, maxpath);
  makewritefds(&writefds, maxpath);
  res=select(dtablesize, &readfds, &writefds, NULL, NULL);
  if ((res==-1) && (errno!=EINTR))
    {
    elog << "mainloop:select 2" << flush;
    return;
    }
  if (res>0)
    {
    doread(&readfds, maxpath);
    dowrite(&writefds, maxpath);
    }
  }
elog<<"second loop finished"<<flush;
leavemainloop=0;

// here we should delete all clients with pol_nowait
size=clientlist.size();
for (i=size-1; i>=0; i--)
  {
  if (clientlist[i]->pol_nwait())
    {
    elog<<"delete client "<<clientlist[i]->name();
    delete clientlist[i];
    }
  }
// execute loop until clientlist and serverlist are empty
elog<<"enter third loop"<<flush;
while (((clientlist.size()>0) || (serverlist.size()>0)) && !leavemainloop)
  {
  int maxpath=-1;
  makereadfds(&readfds, maxpath);
  makewritefds(&writefds, maxpath);
  res=select(dtablesize, &readfds, &writefds, NULL, NULL);
  if ((res==-1) && (errno!=EINTR))
    {
    elog << "mainloop:select 2" << flush;
    return;
    }
  if (res>0)
    {
    doread(&readfds, maxpath);
    dowrite(&writefds, maxpath);
    }
  }
elog<<"third loop finished"<<flush;

}/* mainloop */

/*****************************************************************************/
extern "C" {
void sig_int(int sig)
{/* sig_int */
termsig=sig;
leavemainloop=1;
}/* sig_int */
}
/*****************************************************************************/
extern "C" {
void sig_pipe(int sig)
{
cerr << "got signal sigpipe ("<<sig<<")" << endl;
}
}
/*****************************************************************************/

int main(int argc, char *argv[])
{/* main */
TR(main)
/*try
  {*/
  signal(SIGINT, sig_int);
  signal(SIGTERM, sig_int);
  signal(SIGPIPE, sig_pipe);
  dtablesize=getdtablesize();

  args=new C_readargs(argc, argv);

  if (readargs()!=0) return(0);

  if (background) cdaemon(1,0);
  #ifdef COSYLOG
  if (background)
    {
    lgg_col=new C_cosylogger(argv[0], use_cosylog, "/dev/console");
    lgg.addlog(lgg_col);
    }
  else
    {
    //lgg_col=new C_cosylogger(argv[0], use_cosylog, "/dev/tty");
    //lgg.addlog(lgg_col);
    }
  #endif

  /* write PID file */
  {
    const char *pidname=0;
    FILE *pidfile=0;
    pidname=args->getstringopt("pidfile");
    if (pidname && pidname[0]) {
      pid_t pid;
      pid=getpid();
      pidfile=fopen(pidname, "w");
      if (pidfile) {
        fprintf(pidfile, "%llu\n", (unsigned long long)pid);
        fclose(pidfile);
      }
    }
  }

  lgg.addlog(new C_syslogger(argv[0]));
  lgg.addlog(new C_speciallogger(argv[0]));

  int numflogs=args->multiplicity("logfile");
  if ((numflogs==0) && (!background))
    {
    try
      {
      C_logger* logg;
      logg=new C_filelogger(argv[0], "/dev/tty");
      lgg.addlog(logg);
      }
    catch(C_error* e)
      {
      elog << (*e) << flush;
      delete e;
      }
    }
  else
    for (int flog=0; flog<numflogs; flog++)
      {
      try
        {
        C_logger* logg;
        logg=new C_filelogger(argv[0], args->getstringopt("logfile", flog));
        lgg.addlog(logg);
        }
      catch(C_error* e)
        {
        elog << (*e) << flush;
        delete e;
        }
      }

  nlog << time << "start" << flush;

  #ifdef ODB
  if (use_odb)
    {
    try
      {
      cdb=new C_odb(fdbname, dbname);
      }
    catch(C_error* e)
      {
      elog << "cannot init database" << dbname << (*e) << flush;
      delete e;
      goto raus;
      }
    catch (...)
      {
      cerr << "caught ... in main() in commu.cc (2)" << endl;
      exit;
      }
    }
  else
  #endif
  #ifdef COSYDB
  if (use_cosydb)
    {
    try
      {
      cdb=new C_cdb();
      }
    catch(C_error* e)
      {
      elog << "cannot init database" << (*e) << flush;
      delete e;
      goto raus;
      }
    catch (...)
      {
      cerr << "caught ... in main() in commu.cc (3)" << endl;
      exit(0);
      }
    }
  else
  #endif
  #ifdef NSDB
  if (use_nsdb)
    {
    try
      {
      cdb=new C_nsdb(args->getstringopt("nsprog"),
          args->getstringopt("nshost"), args->getintopt("nsport"));
      }
    catch(C_error* e)
      {
      elog << "cannot init nameserver: " << (*e) << flush;
      delete e;
      goto raus;
      }
    }
  else
  #endif
    {
    try
      {
      cdb=new C_zdb(commlist);
      }
    catch(C_error* e)
      {
      elog << "cannot read combezlist: " << (*e) << flush;
      delete e;
      goto raus;
      }
    }
  if (openusocket()==-1) goto raus;
  if (openisocket()==-1) goto raus;
  forkwatcher();
  mainloop();
  raus:
  delete cdb;
  elog.flushit();
  nlog.flushit();
  dlog.flushit();
  nlog << time << "stop" << grund << flush;
  return(0);
/*  }

catch (C_error* e)
  {
  elog << "Fehler: " << *e << flush;
  delete e;
  }
catch (...)
  {
  cerr << "caught ... in main() in commu.cc" << endl;
  }
*/
}/* main */

/*****************************************************************************/
/*****************************************************************************/
