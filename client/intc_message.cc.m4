/*
 * intc_message.cc.m4  
 * created: 21.03.95 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <intc_message.hxx>
#include <errors.hxx>
#include <conststrings.h>
#include "versions.hxx"

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL intc_message.cc.m4,v 2.7 1998/06/29 17:47:02 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

void C_intc_message::print()
{
C_EMS_message* msg;
IntMsg type;
type=conf.header()->type.intmsgtype;
cout << "intmsg " << Int_str(type) << endl;
  switch (type)
    {
  define(`version', `')
  define(`Intmsg', `
  case Intmsg_$1: msg=new C_$1_ic_message(*this); break;
  ')
  include(COMMON_INCLUDE/intmsgtypes.inc)
    default: msg=new C_intc_unknown_message(*this); break;
    }
  try
    {
    msg->print();
    }
  catch (C_error* e)
    {
  undefine(`index')
    buf.index(0);
    int i;
    int num=buf.size();
    cout << "  Fehler in Nachricht; Daten: " << endl;
    for (i=0; i<num; i++) cout << buf.getint() << " ";
    cout << endl;
    delete e;
    }
  delete msg;
//   }
}

/*****************************************************************************/

void C_intc_unknown_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Nichts_ic_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_ClientIdent_ic_message::print()
{
int err;
buf >> err;
switch (err)
  {
  case EMSE_OK:
    {
    int ident, ver;    
    buf >> ident >> ver;
    cout << "  identifier: "
        << ((ident>>24)&0xff) << '.' << ((ident>>16)&0xff)
        << ": " << (ident&0xffff)
        << "; intmsg_version " << ver << endl;
    }
    break;
  default:
    {
    int code;
    buf >> code;
    cout << "  Error " << err << " code " << code << endl;
    }
    break;
  }
rest(buf);
}

/*****************************************************************************/

void C_Open_ic_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Close_ic_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_OpenVED_ic_message::print()
{
int err;
buf >> err;
switch (err)
  {
  case EMSE_OK:
    {
    int selector;
    buf >> selector;
    cout << "  selector " << selector << endl;
    switch (selector)
      {
      case 0:
        {
        int handle, num, ver_ved, ver_req, ver_unsol;
        buf >> handle >> num;
        cout << "  handle=" << handle << "; ";
        if (num==3)
          {
          buf >> ver_ved >> ver_req >> ver_unsol;
          cout << "ver_ved=" << ver_ved << "; ver_req=" << ver_req
              << "; ver_unsol=" << ver_unsol << endl;
          }
        else
          {
          int i;
          cout << num << " worte: ";
          for (i=0; i<num; i++) cout << buf.getint();
          cout << endl;
          }
        }
        break;
      case 1:
        {
        cout << "  (kann nur unsolicited auftreten \?\?)" << endl;
        int handle, num, ver_ved, ver_req, ver_unsol;
        buf >> handle >> num;
        cout << "  handle=" << handle << "; ";
        if (num==3)
          {
          buf >> ver_ved >> ver_req >> ver_unsol;
          cout << "ver_ved=" << ver_ved << "; ver_req=" << ver_req
              << "; ver_unsol=" << ver_unsol << endl;
          }
        else
          {
          int i;
          cout << num << " worte: ";
          for (i=0; i<num; i++) cout << buf.getint();
          cout << endl;
          }
        }
        break;
      default:
        cout << "  (selector is unknown)" << endl;
        break;
      }
    }
    break;
  case EMSE_InProgress:
    {
    cout << "  " << EMS_errstr(static_cast<EMSerrcode>(err)) << "; ";
    int selector;
    buf >> selector;
    if (selector==0)
      {
      int handle;
      buf >> handle;
      cout << "handle=" << handle << endl;
      }
    else
      cout << "  unknown selector " << selector << endl;
    }
    break;
  default:
    cout << "  Error " << EMS_errstr(static_cast<EMSerrcode>(err)) << endl;
    break;
  }
rest(buf);
}

/*****************************************************************************/

void C_CloseVED_ic_message::print()
{
int err;
buf >> err;
if (err) cout << "  " << EMS_errstr(EMSerrcode(err)) << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetHandle_ic_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_GetName_ic_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_SetUnsolicited_ic_message::print()
{
buf++;
cout << "  old value: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_SetExitCommu_ic_message::print()
{
buf++;
cerr << "  old value: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/
/*****************************************************************************/
