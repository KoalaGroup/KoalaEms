/*
 * intr_message.cc.m4
 * created 21.03.95 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <intr_message.hxx>
#include <conststrings.h>
#include "versions.hxx"

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: intr_message.cc.m4,v 2.7 2007/04/18 19:42:15 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

void C_intr_message::print()
{
C_EMS_message* msg;
IntMsg type;
type=conf.header()->type.intmsgtype;
cout << "intmsg " << Int_str(type) << endl;
switch (type)
  {
define(`version', `')
define(`Intmsg', `
case Intmsg_$1: msg=new C_$1_ir_message(*this); break;
')
include(COMMON_INCLUDE/intmsgtypes.inc)
  default: msg=new C_intr_unknown_message(*this); break;
  }
msg->print();
delete msg;
}

/*****************************************************************************/

void C_intr_unknown_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Nichts_ir_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_ClientIdent_ir_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Open_ir_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Close_ir_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_OpenVED_ir_message::print()
{
STRING name;
buf >> name;
cout << "  " << name << endl;
rest(buf);
}

/*****************************************************************************/

void C_CloseVED_ir_message::print()
{
cout << "  ID: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetHandle_ir_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_GetName_ir_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_SetUnsolicited_ir_message::print()
{
cout << "  ved: "<<buf.getint()<<"; value: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_SetExitCommu_ir_message::print()
{
cout << "  value: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/
/*****************************************************************************/
