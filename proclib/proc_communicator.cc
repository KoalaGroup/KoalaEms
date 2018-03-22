/*
 * proc_communicator.cc
 * 
 * created: 11.06.97
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <msg.h>
#include <clientcomm.h>
#include "proc_communicator.hxx"
#include <ems_errors.hxx>
#include <proc_veds.hxx>
#include <compat.h>
#include <versions.hxx>

#ifndef DEFSOCK
#define DEFSOCK "/var/tmp/emscomm"
#endif

#ifndef DEFPORT
#define DEFPORT 4096
#endif

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_communicator.cc,v 2.12 2016/05/10 16:24:46 wuestner Exp $")
#define XVERSION

int C_communicator::counter=0;
C_communicator communication;

using namespace std;

/*****************************************************************************/

C_communicator::C_communicator()
:pcallback(0), pbackdata(0), mcallback(0), mbackdata(0), contyp(none)
{
if (counter>0)
    throw new C_program_error("can not create a second communicator.");
counter++;
deftimeout(20);
install_ccommback(ccallback, this);
}

/*****************************************************************************/

C_communicator::~C_communicator()
{
if (valid()) Clientcomm_done();
}

/*****************************************************************************/

void C_communicator::ccallback(int action, int reason, int path, void* data)
{
reinterpret_cast<C_communicator*>(data)->ccallbackm(action, reason, path);
}

/*****************************************************************************/

void C_communicator::ccallbackm(int action, int reason, int path)
{
if (pcallback) pcallback(pbackdata, action, reason, path);
}

/*****************************************************************************/

int C_communicator::deftimeout(int val)
{
int s;
s=deftimeout_.tv_sec;
deftimeout_.tv_sec=val?val:20;
deftimeout_.tv_usec=0;
::settimeout(deftimeout_.tv_sec);
return s;
}

/*****************************************************************************/
void C_communicator::init()
{
/*
EMSHOST oder EMSPORT angegeben:
  tcp-Verbindung; default port=4096

EMSSOCKET angegeben:
  lokale Verbindung ueber Unix-Socket

default:
  lokale Verbindung ueber /var/tmp/emscomm
*/

    const char *emshost, *emsport, *emssocket;
    int port;

    emshost=getenv("EMSHOST");
    emsport=getenv("EMSPORT");
    emssocket=getenv("EMSSOCKET");

    if (emshost && emssocket) {
        throw new C_program_error("Can't init communication: "
            "only environment variable EMSHOST or EMSSOCKET is allowed.");
    }

    if (emshost) {
        if (emsport) {
            char dummy[1024];
            istringstream s(emsport);
            if (!(s >> port) || (s >> dummy)) {
                ostringstream s;
                s << "Can't init communication: "
                    << "cannot convert environment variable EMSPORT \""
                    << emsport << "\".";
                throw new C_program_error(s);
            }
        } else {
            port=DEFPORT;
        }
        init(emshost, port);
    } else {
        if (!emssocket)
            emssocket=DEFSOCK;
        init(emssocket);
    }
}
/*****************************************************************************/

void C_communicator::init(const string& socket_)
{
if (Clientcomm_init(socket_.c_str())!=0)
  {
  ostringstream s;
  s << "Can't init communication via unix-socket \"" << socket_
      << "\"";
  throw new C_ems_error(EMS_errno, s);
  }
socket=socket_;
contyp=local;
path_=path();
//if (pcallback) pcallback(path_, 1);
}

/*****************************************************************************/

void C_communicator::init(const string& host_, int port_)
{
if (Clientcomm_init_e(host_.c_str(), port_)!=0)
  {
  ostringstream s;
  s << "Can't init communication via host \"" << host_ << "\", port " << port_;
  throw new C_ems_error(EMS_errno, s);
  }
host=host_;
port=port_;
contyp=tcp;
path_=path();
//if (pcallback) pcallback(path_, 1);
}

/*****************************************************************************/

void C_communicator::done()
{
if (valid()) veds.close();
if (Clientcomm_done()!=0)
    throw new C_ems_error(EMS_errno, "Can't close communication");
contyp=none;
//if (pcallback) pcallback(path_, 0);
}

/*****************************************************************************/

void C_communicator::name(ostream& os) const
{
switch (contyp)
  {
  case none:
    os << "no communication";
    break;
  case local:
    os << "socket " << socket;
    break;
  case tcp:
  case decnet:
    os << "host " << host << "; port " << port;
    break;
  }
}

/*****************************************************************************/

ostream& operator <<(ostream& os, const C_communicator& comm)
{
comm.name(os);
return os;
}

/*****************************************************************************/

int C_communicator::valid() const
{
return(getconfpath()!=-1);
}

/*****************************************************************************/

int C_communicator::path() const
{
return(getconfpath());
}

/*****************************************************************************/
// pcallback wird aufgerufen, wenn die Kommunikation zum commu aufgenommen
// oder beendet wird

C_communicator::pbackproc C_communicator::installpcallback(pbackproc proc,
   void* data)
{
pbackproc oldproc=pcallback;
pcallback=proc;
pbackdata=data;
return oldproc;
}

/*****************************************************************************/
// mcallback wird aufgerufen, wenn nach GetConf noch weitere Messages
// abgeholt werden muessen
// ist notwendig, weil select keine Messages mehr sieht

C_communicator::mbackproc C_communicator::installmcallback(mbackproc proc,
    void* data)
{
mbackproc oldproc=mcallback;
mcallback=proc;
mbackdata=data;
return oldproc;
}

/*****************************************************************************/

C_confirmation* C_communicator::GetConf(struct timeval *timeout)
{
ems_i32 *conf;
int res;
msgheader header;

res=GetConfir(0, 0, 0, &conf, &header, timeout?timeout:&deftimeout_);
if (res==-1) throw new C_ems_error(EMS_errno, "Can't get confirmation");
if (res==0) return 0;
if (mcallback && messagepending()) mcallback(mbackdata);
return new C_confirmation(conf, header, free_confirmation);
}

/*****************************************************************************/
/*****************************************************************************/
