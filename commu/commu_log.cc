/*
 * commu_log.cc
 * 
 * created before 23.02.94
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#include <string.h>
#endif
#include <sys/time.h>
#include <syslog.h>
#include <sys/uio.h> /* wegen struct iovec */
#include <socket_prot.h> /* sys/socket.h, aber geschuetzt (wegen ultrix4.4) */
#include <netinet/in.h>
#include <arpa/inet.h>

#include <debug.hxx>
#include <conststrings.h>
#include <commu_log.hxx>
#include <commu_logg.hxx>
#include <compat.h>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_log.cc,v 2.18 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_log_manipi::C_log_manipi(log_manipi ff, int a)
:f(ff), arg(a)
{}

/*****************************************************************************/

C_log& operator<<(C_log& log, const C_log_manipi& m)
{
return(m.f(log, m.arg));
}

/*****************************************************************************/

C_log::C_log(loggstruct& logger)
:log(logger)
{
sbuffer="";
resetit();
}

/*****************************************************************************/

C_log::~C_log()
{}

/*****************************************************************************/

// void C_log::putit(const char *s)
// {
// //int len, blen;
// // len=strlen(s);
// // blen=strlen(buffer);
// // if (blen+len>=1024) len=1024-blen;
// // strncat(buffer, s, len);
// 
// sbuffer+=s;
// }

/*****************************************************************************/

void C_log::flushit()
{
sbuffer="";
resetit();
}

/*****************************************************************************/

void C_log::resetit()
{
base=10;
printtime=0;
level=0;
}

/*****************************************************************************/

void C_log::time(int t)
{
printtime=t;
}

/*****************************************************************************/

void C_log::setbase(int b)
{
base=b;
}

/*****************************************************************************/

void C_log::setlevel(int t)
{
level=t;
}

/*****************************************************************************/

C_log& C_log::operator<<(int v)
{
    ostringstream ss;
    if (base==16) {
        ss.setf(ios::showbase);
        ss << hex << v << ends;
    } else
        ss << v << ends;
    putit(ss.str());
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(unsigned int v)
{
    ostringstream ss;
    if (base==16) {
        ss.setf(ios::showbase);
        ss << hex << v << ends;
    } else
        ss << v << ends;
    putit(ss.str());
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(const void* v)
{
    ostringstream ss;
    ss << (void*)v << ends;
    putit(ss.str());
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(char v)
{
char s[2];
s[0]=v;
s[1]=0;
putit(s);
return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(const char *v)
{
putit(v);
return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(const string v)
{
    putit(v);
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(const fd_set *fds)
{
    int i, k;
    int count=0;
    ostringstream ss;

#ifdef xxx__linux
    k=1024;
#else
    k=getdtablesize();
#endif
    for (i=0; i<k; i++) {
        if (FD_ISSET(i, fds)) {
            if (count>0) ss << ' ';
            ss << i;
            count++;
        }
    }
    ss << ends;
    putit(ss.str());
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(struct sockaddr_un addr)
{
    ostringstream ss;
    ss<<"sock_family: "<<addr.sun_family<<", path: "<<addr.sun_path<<ends;
    putit(ss.str());
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(struct sockaddr_in addr)
{
    ostringstream ss;
    ss<<"sock_family: "<<addr.sin_family<<", port: "<<ntohs(addr.sin_port)
        <<", host: "<<inet_ntoa(addr.sin_addr)<<ends;
    putit(ss.str());
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(const C_error& error)
{
    ostringstream ss;
    ss << error << ends;
    putit(ss.str());
    return(*this);
}

/*****************************************************************************/

C_log& C_log::operator<<(log_manip f)
{
return(f(*this));
}

/*****************************************************************************/

C_elog::C_elog(loggstruct& logger)
:C_log(logger)
{}

/*****************************************************************************/

C_elog::~C_elog()
{
}

/*****************************************************************************/

void C_elog::flushit()
{
//if (buffer[0]==0) return;
if (sbuffer=="") return;
for (int i=0; i<log.numlogs; i++)
  if (printtime)
    log.logs[i]->lstring_t(LOG_ERR, sbuffer);
  else
    log.logs[i]->lstring(LOG_ERR, sbuffer);
C_log::flushit();
}

/*****************************************************************************/

C_nlog::C_nlog(loggstruct& logger)
:C_log(logger)
{}

/*****************************************************************************/

C_nlog::~C_nlog()
{
}

/*****************************************************************************/

void C_nlog::flushit()
{
//if (buffer[0]==0) return;
if (sbuffer=="") return;
for (int i=0; i<log.numlogs; i++)
  {
  if (printtime)
    log.logs[i]->lstring_t(LOG_NOTICE, sbuffer);
  else
    log.logs[i]->lstring(LOG_NOTICE, sbuffer);
  }
C_log::flushit();
}

/*****************************************************************************/

C_dlog::C_dlog(loggstruct& logger)
:C_log(logger)
{}

/*****************************************************************************/

C_dlog::~C_dlog()
{}

/*****************************************************************************/

void C_dlog::flushit()
{
//if (buffer[0]==0) return;
if (sbuffer=="") return;
#ifdef DEBUG
if ((level & debug)!=0)
  {
  for (int i=0; i<log.numlogs; i++)
    if (printtime)
      log.logs[i]->lstring_t(LOG_DEBUG, sbuffer);
    else
      log.logs[i]->lstring(LOG_DEBUG, sbuffer);
  }
#endif
C_log::flushit();
}

/*****************************************************************************/
/*****************************************************************************/

C_log& setlevel(C_log& log, int t)
{
log.setlevel(t);
return(log);
}

/*****************************************************************************/

C_log_manipi level(const int t)
{
return C_log_manipi(&setlevel, t);
}

/*****************************************************************************/

C_log& seterror(C_log& log, int e)
{
log << ": " << EMS_errstr((EMSerrcode)e);
return(log);
}

/*****************************************************************************/

C_log_manipi error(const int e)
{
return C_log_manipi(&seterror, e);
}

/*****************************************************************************/
/*****************************************************************************/

C_log& flush(C_log& log)
{
log.flushit();
return(log);
}

/*****************************************************************************/

C_log& time(C_log& log)
{
log.time(1);
return(log);
}

/*****************************************************************************/

C_log& hex(C_log& log)
{
log.setbase(16);
return(log);
}

/*****************************************************************************/

C_log& dec(C_log& log)
{
log.setbase(10);
return(log);
}

/*****************************************************************************/

C_log& error(C_log& log)
{
return(seterror(log, (EMSerrcode)errno));
}

/*****************************************************************************/

loggstruct::loggstruct()
:logs(0), numlogs(0)
{}

/*****************************************************************************/

loggstruct::~loggstruct()
{
while (numlogs) dellog(0);
delete[] logs;
}

/*****************************************************************************/

int loggstruct::addlog(C_logger* log)
{
C_logger** help=new C_logger*[numlogs+1];
for (int i=0; i<numlogs; i++) help[i]=logs[i];
delete[] logs;
logs=help;
logs[numlogs]=log;
numlogs++;
return numlogs-1;
}

/*****************************************************************************/

void loggstruct::dellog(int l)
{
delete logs[l];
for (int i=l; i<numlogs-1; i++) logs[i]=logs[i+1];
numlogs--;
}

/*****************************************************************************/
/*****************************************************************************/
