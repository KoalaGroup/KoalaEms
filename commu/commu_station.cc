/*
 * commu_station.cc
 * 
 * created before 14.06.94
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cerrno>
#include <cstdio>
#include <sys/socket.h>
#include "commu_def.hxx"
#include "commu_log.hxx"
#include "commu_station.hxx"
#include "commu_server.hxx"
#include "commu_client.hxx"
#include <config_types.h>
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_station.cc,v 2.22 2014/07/14 15:12:20 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

extern C_clientlist clientlist;
extern C_serverlist serverlist;
extern C_db *cdb;
extern C_log elog, nlog, dlog;
//extern loggstruct lgg;

const char *C_station::io_readfehler   ="read error";
const char *C_station::io_writefehler  ="write error";
const char *C_station::io_readbruch    ="broken pipe during read";
const char *C_station::io_writebruch   ="broken pipe during write";
const char *C_station::io_identfehler  ="identification failed";
const char *C_station::io_disconnect   ="disconnect";
const char *C_station::io_verzicht     ="Client hat aufgegeben";
const char *C_station::io_ende         ="terminate program";
const char *C_station::io_openfehler   ="open rejected";
const char *C_station::io_serverclose  ="no clients left";


/*****************************************************************************/

// char* id_string(int id)
// {
// static char s[1024];
// ostrstream ss(s, 1024);
// TR(id_string)
// switch (id)
//   {
//   case EMS_commu: return("EMS_commu");
//   case EMS_invalid: return("EMS_invalid");
//   case EMS_unknown: return("EMS_unknown");
//   default:
//     if (id & 0xffff0000)
//       {
//       ss<<((id>>24)&0xff)<<'.'<<((id>>16)&0xff)<<'.'<<(id&0xffff)<<ends;
//       ss.str();
//       return(s);
//       }
//     else
//       {
//       ss << id << ends;
//       ss.str();
//       return(s);
//       }
//   }
// }

/*****************************************************************************/

C_ints C_station::logtempl_in(10, "logtempl_in");
C_ints C_station::logtempl_out(10, "logtempl_out");

/*****************************************************************************/

C_station::C_station()
:stationname("DEFAULT"), valid(0), identifier(0), remote(0), policies(pol_none),
    loglist_in(logtempl_in), loglist_out(logtempl_out),
    grund(""), extgrund(""), err(0)
{
TR(C_station::C_station())
// cerr<<"C_station::C_station() logtempl_in:"<<endl;
// cerr<<logtempl_in<<endl;
// cerr<<"C_station::C_station() logtempl_out:"<<endl;
// cerr<<logtempl_out<<endl;
// cerr<<"C_station::C_station() loglist_in:"<<endl;
// cerr<<loglist_in<<endl;
// cerr<<"C_station::C_station() loglist_out:"<<endl;
// cerr<<loglist_out<<endl;
}

/*****************************************************************************/
int C_station::setlog(int client, logtype type)
{
    if (pol_debug()) {
        loglist_in.free(client);    // um doppelte Eintraege zu vermeiden
        loglist_out.free(client);
        switch (type) {
        case log_in:
            loglist_in.add(client);
            break;
            case log_out:
            loglist_out.add(client);
            break;
        case log_bi:
            loglist_in.add(client);
            loglist_out.add(client);
            break;
        case log_none:
            {}
            break;
        }
        return(0);
    } else {
        return -1;
    }
}
/*****************************************************************************/
/*****************************************************************************/

C_io_station::C_io_station()
:status_value(st_non_existent), socket(0), far(0), towrite(0), toread(0),
    messagelist(20, MSGLOWWATER, MSGHIGHWATER), lostmessages(0)
{
TR(C_io_station::C_io_station())
remote=1;
}

/*****************************************************************************/

C_io_station::~C_io_station()
{
TR(C_io_station::~C_io_station)
delete socket;
}

/*****************************************************************************/

const char* C_io_station::statstr() const
{
static const char *str[]=
  {
  "st_non_existent", "st_connecting", "st_opening", "st_read_ident",
  "st_error_ident", "st_ready", "st_disconnecting", "st_close"
  };
if (status()>st_close)
  return("fehler");
else
  return(str[status_value]);
}

/*****************************************************************************/
void C_io_station::read_sel()
{
    TR(C_io_station::read_sel)
    switch (status_value) {
    case st_non_existent:
        work_read_non_existent();
        break;
    case st_connecting:
        work_read_connecting();
        break;
    case st_opening:
        work_read_opening();
        break;
    case st_read_ident:
        work_read_read_ident();
        break;
    case st_error_ident:
        work_read_error_ident();
        break;
    case st_ready:
        work_read_ready();
        break;
    case st_disconnecting:
        work_read_disconnecting();
        break;
    case st_close:
        {} // XXX what do do here? Error?
    }
}
/*****************************************************************************/
void C_io_station::write_sel()
{
    TR(C_io_station::write_sel)
    switch (status_value) {
        case st_non_existent:
        work_write_non_existent();
        break;
    case st_connecting:
        work_write_connecting();
        break;
    case st_opening:
        work_write_opening();
        break;
    case st_read_ident:
        work_write_read_ident();
        break;
    case st_error_ident:
        work_write_error_ident();
        break;
    case st_ready:
        work_write_ready();
        break;
    case st_disconnecting:
        work_write_disconnecting();
        break;
    case st_close:
        {} // XXX what do do here? Error?
    }
}
/*****************************************************************************/
void C_io_station::except_sel()
{
    TR(C_io_station::except_sel)
    elog << "exception on socket " << (*socket) << flush;
    int path=*socket;
    int res, val;
    socklen_type len;
    len=sizeof(val);
    res=getsockopt(path, SOL_SOCKET, SO_ERROR, (char *)&val, &len);
    if (res==0) {
        elog<<"getsockopt(SO_ERROR): "<<strerror(val)<<"("<<val<<")"<<flush;
    } else {
        elog<<"error in getsockopt(SO_ERROR): "<<strerror(errno)
            <<"("<<errno<<")"<<flush;
    }
}
/*****************************************************************************/

int C_io_station::addmessage(C_message *message, int nodel)
{
int res;

TR(C_io_station::addmessage(C_message*))

if (pol_debug() && !(message->header.flags & Flag_NoLog))
    logmessage(message, 1); // alle interessierten clients mit Kopien versorgen

if ((res=messagelist.add(message))==-1) // messagequeue voll
  {
  if (pol_show() && (lostmessages++==0))
      nlog << time << "begin to discard messages for station " << name()
          << lostmessages+1 << " messages lost"
          << flush;
  if (!nodel) delete message;
  }
else
  {
  if (pol_show() && (lostmessages!=0))
    {
    nlog << time << lostmessages << " messages for station " << name()
        << " discarded" << flush;
    lostmessages=0;
    }
  towrite=1;
  }
return(res);
}

/*****************************************************************************/
/*****************************************************************************/

C_local_station::C_local_station()
{
TR(C_local_station::C_local_station())
remote=0;
}

/*****************************************************************************/

C_local_station::~C_local_station()
{
TR(C_local_station::~C_local_station)
}

/*****************************************************************************/

const char* C_local_station::statstr() const
{
return("st_ready");
}

/*****************************************************************************/
/*****************************************************************************/
