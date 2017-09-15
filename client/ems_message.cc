/*
 * ems_message.cc
 * 
 * created: 16.03.95
 * 25.03.1998 PW: adapded for <string>
 * 05.02.1999 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <unistd.h>
#include <signal.h>
#include <ems_message.hxx>
#include <ems_unsol_message.hxx>
#include <emsc_message.hxx>
#include <emsr_message.hxx>
#include <intc_message.hxx>
#include <intr_message.hxx>
#include <errors.hxx>
#include <requesttypes.h>
#include <ved_errors.hxx>
#include <conststrings.h>
#include "versions.hxx"

VERSION("Feb 05 1999", __FILE__, __DATE__, __TIME__,
"$ZEL: ems_message.cc,v 2.12 2004/11/26 22:20:37 wuestner Exp $")
#define XVERSION

C_VED_versions* ved_versions;

/*****************************************************************************/

C_EMS_message::C_EMS_message(const C_confirmation* conf, int loop, int extr)
:conf(*conf), extr(extr), server("MISTa"), client("MISTa"), loop(loop+1)
{
if (loop>20)
  {
  cerr << "loop>20" << endl;
  kill(getpid(), SIGTRAP);
  }
if (conf->header()==0) throw(0);
}

/*****************************************************************************/

C_EMS_message::C_EMS_message(int* conf, msgheader header, int loop, int extr)
:conf(conf, header), extr(extr), server("MISTb"), client("MISTb"), loop(loop+1)
{
if (loop>20)
  {
  cerr << "loop>20" << endl;
  kill(getpid(), SIGTRAP);
  }
}

/*****************************************************************************/

C_EMS_message::C_EMS_message(const C_EMS_message& msg, int extr)
:conf(msg.conf), extr(extr), reqver(msg.reqver), unsolver(msg.unsolver),
    server(msg.server), client(msg.client), loop(msg.loop+1)
{
if (loop>20)
  {
  cerr << "loop>20" << endl;
  kill(getpid(), SIGTRAP);
  }
}

/*****************************************************************************/

C_EMS_message::~C_EMS_message()
{}

/*****************************************************************************/

void C_EMS_message::print()
{
if (!extr) cout
<< "--------------------------------------------------------------------------"
<< endl;

try
  {
  C_EMS_message* msg;
  if (conf.header()->flags & Flag_Intmsg)
    msg=new C_int_message(*this);
  else if ((conf.header()->flags & Flag_Unsolicited))
    msg=new C_unsol_message(*this);
  else
    msg=new C_ems_message(*this);
  msg->print();
  delete msg;
  }
catch (C_error* e)
  {
  cout << endl << (*e) << endl;
  delete e;
  }
}

/*****************************************************************************/

void C_EMS_message::rest(C_inbuf& buf)
{
int num;
if ((num=buf.size()-buf.index())==0) return;
if (num<0)
  {
  cout << "Fehler in rest(C_inbuf&); index=" << buf.index() << ", size="
      << buf.size() << endl;
  return;
  }
cout << "  weitere Daten im Buffer:" << endl;
cout << hex << buf << dec << endl;
//for (i=0; i<num; i++) cout << buf.getint() << " ";
//cout << endl;
}

/*****************************************************************************/

C_int_message::C_int_message(const C_EMS_message& msg)
:C_EMS_message(msg, 0)
{}

/*****************************************************************************/

C_int_message::~C_int_message(void)
{}

/*****************************************************************************/

void C_int_message::print()
{
C_EMS_message* msg;
if (conf.header()->flags&Flag_Confirmation)
  msg=new C_intc_message(*this);
else
  msg=new C_intr_message(*this);
msg->print();
delete msg;
}

/*****************************************************************************/

C_unsol_message::C_unsol_message(const C_EMS_message& msg)
:C_EMS_message(msg, 0), buf(msg.buffer(), msg.size())
{}

/*****************************************************************************/

C_unsol_message::C_unsol_message(const C_unsol_message& msg)
:C_EMS_message(msg, 0), buf(msg.buffer(), msg.size())
{}

/*****************************************************************************/

C_unsol_message::~C_unsol_message(void)
{}

/*****************************************************************************/

void C_unsol_message::print()
{
C_EMS_message* msg;
UnsolMsg type;

ved_versions->getversions("commu_core", reqver, unsolver);
type=conf.header()->type.unsoltype;
if (type!=Unsol_LogMessage)
  {
  cout << "unsolmsg " << Unsol_str(type);
  if (unsolver==-1) cout << " (?)";
  cout << endl;
  }
else if (unsolver==-1)
  {
  cout <<"=== unsol msg type "<<(int)type<<"; unknown version"<<endl;
  }
switch (type)
  {
  case Unsol_Nichts:           msg=new C_unsol_nichts_message(*this); break;
  case Unsol_ServerDisconnect: msg=new C_unsol_disco_message(*this); break;
  case Unsol_Bye:              msg=new C_unsol_bye_message(*this); break;
  case Unsol_Test:             msg=new C_unsol_test_message(*this); break;
  case Unsol_LAM:              msg=new C_unsol_lam_message(*this); break;
  case Unsol_LogText:          msg=new C_unsol_logtext_message(*this); break;
  case Unsol_LogMessage:       msg=new C_unsol_logmsg_message(*this); break;
  case Unsol_RuntimeError:     msg=new C_unsol_rterr_message(*this); break;
  case Unsol_Data:             msg=new C_unsol_data_message(*this); break;
  case Unsol_Text:             msg=new C_unsol_text_message(*this); break;
  case Unsol_Warning:          msg=new C_unsol_warn_message(*this); break;
  case Unsol_Patience:         msg=new C_unsol_patience_message(*this); break;
  default:                     msg=new C_unsol_unknown_message(*this); break;
  }
msg->print();
delete msg;
}

/*****************************************************************************/

C_ems_message::C_ems_message(const C_EMS_message& msg)
:C_EMS_message(msg, 0)
{}

/*****************************************************************************/

C_ems_message::C_ems_message(const C_ems_message& msg)
:C_EMS_message(msg, 0)
{}

/*****************************************************************************/

C_ems_message::~C_ems_message(void)
{}

/*****************************************************************************/

void C_ems_message::print()
{
C_EMS_message* msg;
if (conf.header()->flags&Flag_Confirmation)
  msg=new C_emsc_message(*this);
else
  msg=new C_emsr_message(*this);
msg->print();
delete msg;
}

/*****************************************************************************/

//C_VED_versions::C_VED_versions(C_VED* commu_i)
//:commu_i(commu_i), versroot(0), verstop(0)
C_VED_versions::C_VED_versions(C_instr_system* is_i)
:is_i(is_i), versroot(0), verstop(0)
{
versroot=new vedver;
verstop=versroot;
verstop->name="commu_core";
verstop->reqver=ver_requesttab;
verstop->unsolver=ver_unsolmsgtab;
verstop->next=0;
}

/*****************************************************************************/

C_VED_versions::~C_VED_versions()
{
vedver* help;

while (versroot)
  {
  help=versroot;
  versroot=versroot->next;
  delete help;
  }
}

/*****************************************************************************/

int C_VED_versions::getver(const STRING& ved)
{
int res=-1;
try
  {
  C_confirmation* conf;
#ifdef USE_STRING_H
  conf=is_i->command("VEDinfo", ved);
#else
  conf=is_i->command("VEDinfo", ved.c_str());
#endif
  C_inbuf inbuf(conf->buffer(), conf->size());
  inbuf+=2;              // errorcode, ident
  if (inbuf.getint()==5) // status
    {
    int reqv, unsolv;
    inbuf++;             // VED version
    reqv=inbuf.getint();
    unsolv=inbuf.getint();
    cout<<"(VED "<<ved<<" reqver="<<reqv<<"; unsolver="<<unsolv<<')'<<endl;
    vedver* help=new vedver;
    if (verstop)
      verstop->next=help;
    else
      versroot=help;
    verstop=help;
    verstop->name=ved;
    verstop->reqver=reqv;
    verstop->unsolver=unsolv;
    verstop->next=0;
    res=0;
    }
  else
    {
    cout << "(VED "<<ved<<" nicht ready; cannot get version numbers)" << endl;
    }
  delete conf;
  }
catch(C_error* error)
  {
  cout << "(C_VED_versions::getver: " << (*error) <<')'<< endl;
  delete error;
  }
return res;
}

/*****************************************************************************/

int C_VED_versions::getversions(const STRING& name, int& reqver, int& unsolver)
{
vedver* help=versroot;
while (help && (help->name!=name)) help=help->next;
if (help)
  {
  reqver=help->reqver;
  unsolver=help->unsolver;
  return 0;
  }
if (getver(name)!=0)
  {
  reqver=-1;
  unsolver=-1;
  return -1;
  }
reqver=verstop->reqver;
unsolver=verstop->unsolver;
return 0;
}
/*****************************************************************************/

Request C_VED_versions::request(Request req, int ver) const
{
return(req);
}

/*****************************************************************************/

UnsolMsg C_VED_versions::unsol(UnsolMsg msg, int ver) const
{
return(msg);
}

/*****************************************************************************/
/*****************************************************************************/
