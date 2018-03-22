/*
 * proc_dataoutstatus.cc
 * created: 12.06.97
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <errno.h>
#include <stdlib.h>
#include <objecttypes.h>
#include "proc_dataoutstatus.hxx"
#include "proc_conf.hxx"
#include <errors.hxx>
#include <versions.hxx>

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_dataoutstatus.cc,v 2.11 2016/05/10 16:24:46 wuestner Exp $")
#define XVERSION

using namespace std;

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
// C_dataoutstatus* st;
// st=reinterpret_cast<C_dataoutstatus*>(&status);
// return st->print(os);

C_dataoutstatus st(status);
return st.print(os);
}

/*****************************************************************************/
/*****************************************************************************/
