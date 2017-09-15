/*
 * microeventrate_tcl.cc
 * 
 * created: 31.10.95
 * 25.03.1998 PW: adapded for <string>
 * 29.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"

#include <unistd.h>
//#include <stdarg.h>
#include <netinet/in.h>
#include <time.h>
#include <readargs.hxx>
#include <ems_errors.hxx>
#include <ved_errors.hxx>
#include <proc_communicator.hxx>
#include <proc_ved.hxx>
#include <cerrno>
#include <csignal>
#include <sockets.hxx>
#include <errors.hxx>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <compat.h>
#include <client_defaults.hxx>
#include <versions.hxx>

VERSION("Feb 02 2005", __FILE__, __DATE__, __TIME__,
"$ZEL: microeventrate.cc,v 2.3 2010/09/10 22:22:28 wuestner Exp $")
#define XVERSION

C_VED* ved;
C_readargs* args;
int running, try_reopen;
tcp_socket* sock;
int use_dp;
int use_socket;
int use_raw_time;
int test, printstamps, nomist;
void (*show_data)(struct timeval&, int, InvocStatus, float);

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
    args->addoption("interval", "i", 10, "interval in mseconds", "interval");
    args->addoption("localtime", "localtime", false,
        "don't use timestamps from server", "localtime");
    args->addoption("vedname", 0, "", "Name des VED", "vedname");
    args->addoption("name", 1, "", "Name des darzustellenden Wertes", "name");
    args->addoption("old", "old", false, "use old xhisto");
    args->addoption("remote", "remote", false,
        "use remote display");
    args->addoption("xhost", "xhost", "localhost", "host where remote display runs",
        "xhost");
    args->addoption("xport", "xport", 3333, "port for remote display", "xport");
    args->addoption("xsock", "xsock", "/var/tmp/evrate",
        "socket for remote display", "xport");
    //args->addoption("logfile", "lfile", "evrate", "logfile name", "logfile");
    //args->addoption("loginterval", "linterval", 60, "log interval", "loginterval");
    //args->addoption("log", "log", false, "use log");
    //args->addoption("rawtime", "rt", false, "write raw time in logfile");
    args->addoption("test", "test", false, "no connection to server");
    args->addoption("stamps", "z", false, "print timestamps");
    args->addoption("noprint", "n", false, "don't print if not running");

    args->hints(C_readargs::required, "vedname", 0);
    args->hints(C_readargs::exclusive, "xhost", "xsock", 0);
    args->hints(C_readargs::exclusive, "xport", "xsock", 0);

    args->hints(C_readargs::implicit, "xhost", "remote", 0);
    args->hints(C_readargs::implicit, "xport", "remote", 0);
    args->hints(C_readargs::implicit, "xsock", "remote", 0);
    //args->hints(C_readargs::implicit, "logfile", "log", 0);
    //args->hints(C_readargs::implicit, "loginterval", "log", 0);

    //args->hints(C_readargs::exclusive, "log", "remote", 0);

    if (args->interpret(1)!=0) return -1;
    use_dp=!args->getboolopt("old");
    use_socket=args->getboolopt("remote");
    //use_raw_time=args->getboolopt("rawtime");
    test=args->getboolopt("test");
    return 0;
}
/*****************************************************************************/
void opensocket()
{
    sock=new tcp_socket("data");
    try {
        sock->create();
        sock->connect(args->getstringopt("xhost"), args->getintopt("xport"));
    } catch (...) {
        delete sock;
        throw;
    }
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
        try_reopen=0;
    }
}
/*****************************************************************************/
void sendbytes(const char* b, int num)
{
    int sent=0, res;
    do {
        if ((res=write(sock->path(), b+sent, num-sent))<0) {
            if (errno!=EINTR) throw new C_unix_error(errno, "write to socket");
        } else {
            sent+=res;
        }
    } while (sent<num);
}
/*****************************************************************************/
void sende(int sec, int usec, float val)
{
    if (use_dp) {
        OSTRINGSTREAM ss;
        double time;
        time=static_cast<double>(sec)+static_cast<double>(usec)/1000000.0;
        ss << setprecision(20) << time << ' ' << val << endl;
        STRING s=ss.str();
        sendbytes(s.data(), s.length());
    } else {
        int ntime, nval;
        union {
            int iv;
            float fv;
        } fi;

        ntime=htonl(sec);
        fi.fv=val;
        nval=htonl(fi.iv);
        sendbytes(reinterpret_cast<char*>(&ntime), 4);
        sendbytes(reinterpret_cast<char*>(&nval), 4);
    }
}
/*****************************************************************************/
#if 0
void sende_name(const char* name)
{
    OSTRINGSTREAM ss;
    ss << "name " << name << endl;
    STRING s=ss.str();
    sendbytes(s.c_str(), s.length());
}
#else
void sende_name(const char* name)
{
    STRING s("name ");
    s+=name;
    s+='\n';
    sendbytes(s.data(), s.length());
}
#endif
/*****************************************************************************/

void show_data_print(struct timeval& jetzt, int event, InvocStatus status,
    float rate)
{
static InvocStatus laststate=static_cast<InvocStatus>(100);
static int nochange=0;
static float lastrate=0;

static const char *Invoc_text[]={
  "Fehler          ",
  "Daten sind alle ",
  "gestoppt        ",
  "inaktiv         ",
  "rennt           ",
  };

if (!nomist || (rate>0) || (rate!=lastrate) || (status!=laststate))
  {
  if (nochange>1) cout << endl;
  if (printstamps)
    {
    time_t tt;
    struct tm *tm;
    char s[1024];
    tt=jetzt.tv_sec;
    tm=localtime(&tt);
    strftime(s, 1024, "%D %H:%M:%S", tm);
    cout << s << ": ";
    } 
  if ((status>=Invoc_error) && (status<=Invoc_active))
    cout << Invoc_text[status-Invoc_error];
  else
    cout << "status unbekannt: " << static_cast<int>(status);
  cout << "event " << event;

  if (rate>=0)
    {
    cout.precision(2);
    cout.setf(ios::fixed, ios::floatfield);
    cout.width(9);
    cout << ' ' << rate << " Events/s";
    }
  cout << endl;
  nochange=0;
  }
else if (nochange==0)
  {
  cout << "--- wait for status change ---" << endl;
  nochange++;
  }
else
  {
  cout << '.' << flush;
  nochange++;
  }
lastrate=rate;
laststate=status;
}

/*****************************************************************************/

void show_data_send(struct timeval& jetzt, int event, InvocStatus status,
    float rate)
{
if (rate>=0) sende(jetzt.tv_sec, jetzt.tv_usec, rate);
}

/*****************************************************************************/
static int
rateloop(int interval/*ms*/)
{
    int first, lastok;
    int servertime;
    static C_readoutstatus* laststatus=0;
    static C_readoutstatus* newstatus=0;
    InvocStatus state;
    const int eins=1;
    time_t sec=interval/1000;
    long  nsec=(interval%1000)*1000000;
 
    static int Invoc_ok[]={
        0, /* Fehler */
        1, /* Daten sind alle */
        1, /* gestoppt */
        0, /* inaktiv */
        1, /* rennt */
    };

    if (laststatus) {delete laststatus; laststatus=0;}
    if (newstatus)  {delete newstatus;  newstatus=0; }

    if (test) {
        servertime=0;
        srandom(17);
    } else {
        if (args->getboolopt("localtime")) {
            servertime=0;
        } else {
            try {
                C_readoutstatus* status=ved->GetReadoutStatus(1, &eins, 0);
                if (status->time_valid()) {
                    cerr << "timestamps from server."  << endl;
                    servertime=1;
                } else {
                    cerr << "no timestamps from server, but no error(?)."  << endl;
                    servertime=0;
                }
                delete status;
            }
            catch (C_error* error) {
                delete error;
                cerr << "no timestamps from server."  << endl;
                servertime=0;
            }
        } try {
            STRING exp;
            exp=ved->GetReadoutParams();
            cout << "owner: " << exp << endl;
        } catch (C_error* error) {
            cerr << (*error) << endl;
            delete error;
        }
    }

    first=1;
    lastok=0;
    running=1;
    try_reopen=0;

    while (running) {
        if (!first) {
            struct timespec rqtp;
            rqtp.tv_sec=sec;
            rqtp.tv_nsec=nsec;
            nanosleep(&rqtp, 0);
        } else {
            first=0;
        }
        try {
            int ok;
            float rate;
            struct timeval jetzt;

            if (test) {
                gettimeofday(&jetzt, 0);
                newstatus=new C_readoutstatus(Invoc_active,
                        (jetzt.tv_sec%1000000)*100+random()%90, jetzt);
            } else {
                newstatus=ved->GetReadoutStatus(servertime?1:0, &eins, 0);
            }
            gettimeofday(&jetzt, 0);
            if (!servertime) {
                newstatus->settime(jetzt);
            }
            state=newstatus->status();
            if ((state>=Invoc_error) && (state<=Invoc_active))
                ok=Invoc_ok[state-Invoc_error];
            else
                ok=0;
            if (ok && lastok)
                rate=newstatus->evrate(*laststatus);
            else
                rate=-1;
            show_data(jetzt, newstatus->eventcount(), state, rate);
            lastok=ok;
            delete laststatus;
            laststatus=newstatus;
        } catch (C_unix_error* error) {
            if (error->error()==EPIPE) throw;
            cerr << (*error) << endl;
            delete error;
        }
        catch (C_ved_error* error) {
            cerr << (*error) << endl;
            if ((error->errtype()==C_ved_error::req_err)
                    && (error->error().req_err==Err_NoVED)) {
                running=0;
                try_reopen=1;
            }
            delete error;
        } catch (C_error* error) {
            cerr << (*error) << endl;
            delete error;
        }
    }
    return(0);
}
/*****************************************************************************/
int openved(const char* name)
{
if (test) return 0;

int loop, count=0;
int olderrclass=0, errclass;
int olderrtype=0, errtype;
int olderrcode=0, errcode;
C_error* e=0;

do
  {
  if (count++>0) sleep(15);
  try
    {
    ved=new C_VED(name);
    if (count>1) cerr << endl << "VED " << name << " open" << endl;
    return 0;
    }
  catch (C_ved_error* error)
    {
    errclass=error->type();
    errtype=error->errtype();
    errcode=error->error().syserr;
    e=error;
    loop=try_reopen;
    }
  catch (C_ems_error* error)
    {
    errclass=error->type();
    errtype=0;
    errcode=error->code();
    e=error;
    loop=try_reopen;
    }
  catch (C_error* error)
    {
    errclass=error->type();
    errtype=0;
    errcode=0;
    e=error;
    loop=0;
    }
  if ((errclass!=olderrclass) || (errtype!=olderrtype) || (errcode!=olderrcode))
    {
    cerr << (*e) << endl;
    olderrclass=errclass;
    olderrtype=errtype;
    olderrcode=errcode;
    count=0;
    }
  else
    {
    cerr << '.' << flush;
    }
  count++;
  delete e;
  }
while (loop);
return -1;
}

/*****************************************************************************/

void delete_ved()
{
if (test) return;
delete ved;
}

/*****************************************************************************/
int
main(int argc, char* argv[])
{
args=new C_readargs(argc, argv);
if (readargs()!=0) return(0);

printstamps=args->getboolopt("stamps");
nomist=args->getboolopt("noprint");
if (!test)
  {
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
    }
  catch(C_error* error)
    {
    cerr << (*error) << endl;
    delete error;
    return(0);
    }
  }

struct sigaction vec, ovec;

if (sigemptyset(&vec.sa_mask)==-1)
  {
  cerr << "sigemptyset: " << strerror(errno) << endl;
  return(-1);
  }
vec.sa_flags=0;
vec.sa_handler=alarmhand; /* LINUX!! */
if (sigaction(SIGALRM, &vec, &ovec)==-1)
  {
  cerr << "sigaction: " << strerror(errno) << endl;
  return(-1);
  }
vec.sa_handler=inthand; /* LINUX!! */
if (sigaction(SIGINT, &vec, &ovec)==-1)
  {
  cerr << "sigaction: " << strerror(errno) << endl;
  return(-1);
  }
  
try_reopen=1;
try
  {
  if (use_socket)
    {
    opensocket();
    if (use_dp)
      {
      if (args->isdefault("name"))
        sende_name(args->getstringopt("vedname"));
      else
        sende_name(args->getstringopt("name"));
      }
    show_data=show_data_send;
    }
  else
    show_data=show_data_print;
  while (try_reopen && (openved(args->getstringopt("vedname"))==0))
    {
    rateloop(args->getintopt("interval"));
    delete_ved();
    }
  }
catch (C_error* e)
  {
  cerr << (*e) << endl;
  delete e;
  }
  
communication.done();

cout << "ende" << endl;
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
