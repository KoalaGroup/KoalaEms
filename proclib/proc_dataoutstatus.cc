/*
 * proc_dataoutstatus.cc
 * created: 12.06.97
 * 22.03.1999 PW: device eliminated
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errno.h>
#include <stdlib.h>
#include <objecttypes.h>
#include "proc_dataoutstatus.hxx"
#include "proc_conf.hxx"
#include <errors.hxx>
#include <versions.hxx>

VERSION("Mar 03 1999", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_dataoutstatus.cc,v 2.9 2004/11/26 14:44:27 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_dataoutstatus::C_dataoutstatus(const C_confirmation* conf)
:buf(conf->buffer(), conf->size())
{}

/*****************************************************************************/

C_dataoutstatus::~C_dataoutstatus(void)
{}

/*****************************************************************************/

// ostream& C_dataoutstatus::print(ostream& os)
// {
// buf.index(0);
// buf++;
// int error, fertig, disabled;
// buf >> error >> fertig >> disabled;
// os << "{error: " << error << "} {status: ";
// switch ((DoRunStatus)fertig)
//   {
//   case Do_running: os << "running"; break;
//   case Do_neverstarted: os << "never_started"; break;
//   case Do_done: os << "done"; break;
//   case Do_error: os << "error"; break;
//   default: os << fertig; break;
//   }
// os << "} {switch: ";
// switch ((DoSwitch)disabled)
//   {
//   case Do_noswitch: os << "noswitch"; break;
//   case Do_enabled: os << "enabled"; break;
//   case Do_disabled: os << "disabled"; break;
//   default: os << disabled; break;
//   }
// os << "}" << hex << setiosflags(ios::showbase);
// while (!buf.empty())
//   {
//   os << ' ' << buf.getint();
//   }
// return os;
// }

ostream& C_dataoutstatus::print(ostream& os)
{
buf.index(0);
buf++;
int error, fertig, disabled;
buf >> error >> fertig >> disabled;
os<<error<<' '<<fertig<<' '<<disabled;
os << hex << setiosflags(ios::showbase);
while (!buf.empty())
  {
  os << ' ' << buf.getint();
  }
return os;
}

/*****************************************************************************/

ostream& operator <<(ostream& os, const C_dataoutstatus& status)
{
C_dataoutstatus* st;
st=(C_dataoutstatus*)&status;
return st->print(os); 
}

/*****************************************************************************/
/*****************************************************************************/
