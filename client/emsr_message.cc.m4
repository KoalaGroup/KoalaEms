/*
 * emsr_message.cc.m4  
 * created: 18.03.95 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <emsr_message.hxx>
#include <errors.hxx>
#include <conststrings.h>
#include "versions.hxx"

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: emsr_message.cc.m4,v 2.10 2007/04/18 19:42:15 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

void C_emsr_message::print()
{
C_EMS_message* msg;
Request type;

ved_versions->getversions(server, reqver, unsolver);
type=ved_versions->request(conf.header()->type.reqtype, reqver);
cout << Req_str(type);
if (reqver==-1) cout <<" (?)";
cout << endl;
switch (type)
  {
define(`version', `')
define(`Req', `
case Req_$1: msg=new C_$1_r_message(*this); break;
')
include(COMMON_INCLUDE/requesttypes.inc)
  default: msg=new C_emsr_unknown_message(*this); break;
  }
msg->print();
delete msg;
}

/*****************************************************************************/

void C_emsr_unknown_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Nichts_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Initiate_r_message::print()
{
cout << "  ID: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_Identify_r_message::print()
{
cout << "  level " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_Conclude_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_ResetVED_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_GetVEDStatus_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_GetNameList_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_GetCapabilityList_r_message::print()
{
Capabtyp typ=static_cast<Capabtyp>(buf.getint());
cout << "  typ " << Capabtyp_str(typ) << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetProcProperties_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_DownloadDomain_r_message::print()
{
int dump=0;
Domain dom=static_cast<Domain>(buf.getint());
int id=buf.getint();
int size=buf.size()-2;

cout<<"  typ "<<Domain_str(dom)<<"; id "<<id;
switch (dom)
  {
  case Dom_Modullist:
    {
    int num=buf.getint();
    if (num*2+1!=size)
      cout << "; size="<<size<<"; modules="<<num<<" --> wrong format" <<endl;
    else
      {
      cout << "; " << num << " modules:" << endl;
      for (int i=0; i<num; i++)
          cout << "  " << buf.getint() << ": "
              << hex << setiosflags(ios::showbase)
              << buf.getint() << dec << resetiosflags(ios::showbase) << endl;
      }
    }
    break;
  case Dom_Dataout:
    {
    cout << endl <<"  "<< InOutTyp_str(static_cast<InOutTyp>(buf.getint())) << "; size "
        << hex << setiosflags(ios::showbase) << buf.getint()
        << dec << resetiosflags(ios::showbase)
        << "; prior " << buf.getint() << endl;
    IOAddr addr=static_cast<IOAddr>(buf.getint());
    cout << "  " << IOAddr_str(addr);
    switch (addr)
      {
      case Addr_Tape:
      case Addr_LocalSocket:
      case Addr_File:
        {
        STRING s;
        buf >> s;
        cout << " " << s << endl;
        }
        break;
      case Addr_Raw:
      case Addr_Modul:
      case Addr_Driver_mapped:
      case Addr_Driver_mixed:
      case Addr_Driver_syscall:
      case Addr_Socket:
      case Addr_Null:
      default:
        break;
      }
    dump=1;
    }
    break;
  case Dom_Datain:
    {
    cout << endl <<"  "<< InOutTyp_str(static_cast<InOutTyp>(buf.getint())) << endl;
    IOAddr addr=static_cast<IOAddr>(buf.getint());
    cout << "  " << IOAddr_str(addr);
    switch (addr)
      {
      case Addr_Tape:
      case Addr_LocalSocket:
      case Addr_File:
        {
        STRING s;
        buf >> s;
        cout << " " << s << endl;
        }
        break;
      case Addr_Driver_mapped:
      case Addr_Driver_mixed:
      case Addr_Driver_syscall:
        {
        STRING s;
        buf >> s;
        cout << " " << s << endl;
        }
        break;
      case Addr_Raw:
      case Addr_Modul:
      case Addr_Socket:
      case Addr_Null:
      default:
        break;
      }
    dump=1;
    }
    break;
  case Dom_LAMproclist:
    {
    cout << "; sendunsol: " << buf.getint() << "; ";
    int num=buf.getint();
    cout << num << " procs:" << endl;
    for (int i=0; i<num; i++)
      {
      cout << "  proc " << buf.getint();
      int num=buf.getint();
      cout << "; " << num << " args(hex):"
          << hex << setiosflags(ios::showbase);
      for (int j=0; j<num; j++) cout << ' ' <<buf.getint();
      cout << dec << resetiosflags(ios::showbase) << endl;
      }
    }
    break;
  case Dom_null:
  case Dom_Trigger:
  case Dom_Event:
  default:
    dump=1;
    break;
  }
if (dump)
  {
  cout <<"  "<<size<<" words:"<<endl<<"  ";
  cout << hex << setiosflags(ios::showbase);
  for (int i=0; i<size; i++)
    {
    cout << buf[i+2];
    if (i+1<size) cout << ' ';
    }
  cout << dec << resetiosflags(ios::showbase) << endl;
  }
else
  rest(buf);
}

/*****************************************************************************/

void C_UploadDomain_r_message::print()
{
Domain dom=static_cast<Domain>(buf.getint());
int id=buf.getint();
cout<<"  "<<Domain_str(dom)<<"; id "<<id<<endl;
rest(buf);
}

/*****************************************************************************/

void C_DeleteDomain_r_message::print()
{
Domain dom=static_cast<Domain>(buf.getint());
int id=buf.getint();
cout<<"  typ "<<Domain_str(dom)<<"; id "<<id<<endl;
rest(buf);
}

/*****************************************************************************/

void C_CreateProgramInvocation_r_message::print()
{
Invocation inv=static_cast<Invocation>(buf.getint());
cout << "  " << Invocation_str(inv) << "; idx: " << buf.getint() << endl;
switch (inv)
  {
  case Invocation_LAM:
    {
    int num;
    cout << "  id: " <<buf.getint()<<"; is: "<<buf.getint()<<"; ";
    num=buf.getint();
    cout << num << " args(hex):"<<hex;
    for (int i=0; i<num; i++) cout << ' ' << buf.getint();
    cout << dec << endl; 
    }
    break;
  default:
    break;
  }
rest(buf);
}

/*****************************************************************************/

void C_DeleteProgramInvocation_r_message::print()
{
cout << "  " << Invocation_str(static_cast<Invocation>(buf.getint()))
    << "; idx: " << buf.getint() << endl;
if (!buf.empty())
  {
  STRING exp;
  buf >> exp;
  cout << "  Experimentname: " << exp << endl;
  }
rest(buf);
}

/*****************************************************************************/

void C_StartProgramInvocation_r_message::print()
{
cout << "  " << Invocation_str(static_cast<Invocation>(buf.getint()))
    << "; idx: " << buf.getint() << endl;
if (!buf.empty())
  {
  STRING exp;
  buf >> exp;
  cout << "  Experimentname: " << exp << endl;
  }
rest(buf);
}

/*****************************************************************************/

void C_ResetProgramInvocation_r_message::print()
{
cerr << "  " << Invocation_str(static_cast<Invocation>(buf.getint()))
    << "; idx: " << buf.getint() << endl;
if (!buf.empty())
  {
  STRING exp;
  buf >> exp;
  cout << "  Experimentname: " << exp << endl;
  }
rest(buf);
}

/*****************************************************************************/

void C_ResumeProgramInvocation_r_message::print()
{
cerr << "  " << Invocation_str(static_cast<Invocation>(buf.getint()))
    << "; idx: " << buf.getint() << endl;
if (!buf.empty())
  {
  STRING exp;
  buf >> exp;
  cout << "  Experimentname: " << exp << endl;
  }
rest(buf);
}

/*****************************************************************************/

void C_StopProgramInvocation_r_message::print()
{
cerr << "  " << Invocation_str(static_cast<Invocation>(buf.getint()))
    << "; idx: " << buf.getint() << endl;
if (!buf.empty())
  {
  STRING exp;
  buf >> exp;
  cout << "  Experimentname: " << exp << endl;
  }
rest(buf);
}

/*****************************************************************************/

void C_GetProgramInvocationAttr_r_message::print()
{
cerr << "  " << Invocation_str(static_cast<Invocation>(buf.getint()))
    << "; idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetProgramInvocationParams_r_message::print()
{
cerr << "  " << Invocation_str(static_cast<Invocation>(buf.getint()))
    << "; idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_CreateVariable_r_message::print()
{
cout << "  idx: " << buf.getint() << "; size=" << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_DeleteVariable_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_ReadVariable_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_WriteVariable_r_message::print()
{
cout << "  idx: " << buf.getint() << "; ";
int num=buf.getint();
cout << num << " words(hex):"<< endl << hex << setiosflags(ios::showbase);
for (int i=0; i<num; i++)
  {
  cout << ' ' << buf.getint();
  if (i%10==9) cout << endl;
  }
cout << dec << resetiosflags(ios::showbase) << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetVariableAttributes_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_CreateIS_r_message::print()
{
cout << "  idx: " << buf.getint() << "; ID: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_DeleteIS_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_DownloadISModulList_r_message::print()
{
cout << "  IS: " << buf.getint() << "; ";
int num=buf.getint();
cout << num << " module:" << endl << "  ";
for (int i=0; i<num; i++)
  {
  cout << buf.getint();
  if (i+1<num) cout << ' ';
  }
cout << endl;
rest(buf);
}

/*****************************************************************************/

void C_UploadISModulList_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_DeleteISModulList_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_DownloadReadoutList_r_message::print()
{
cout << "  IS: " << buf.getint() << "; prior: " << buf.getint() << endl;
int nt=buf.getint();
cout << "  " << nt << " trigger:";
for (int i=0; i<nt; i++) cout << ' '<< buf.getint();
int num=buf.getint();
cout << "  " << num << " procs" << endl;
for (int j=0; j<num; j++)
  {
  cout << "  proc " << buf.getint();
  int n=buf.getint();
  if (n==0)
    cout << "; no args" << endl;
  else
    {
    cout << "; " << n << " args(hex):" << hex;
    for (int k=0; k<n; k++) cout << ' ' << buf.getint();
    cout << dec << endl;
    }
  }
rest(buf);
}

/*****************************************************************************/

void C_UploadReadoutList_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_DeleteReadoutList_r_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_EnableIS_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_DisableIS_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetISStatus_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetISParams_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_ResetISStatus_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_TestCommand_r_message::print()
{
try
  {
  int pnum;
  cout << "  IS " << buf.getint() <<"; "<<(pnum=buf.getint())<<" procs"<< endl;
  for (int p=0; p<pnum; p++)
    {
    int proc=buf.getint();
    int num=buf.getint();
    cout << "  proc "<<proc<<"; ";
    if (num)
      cout<<num<<" args: " << hex;
    else
      cout<<"no args";
    for (int i=0; i<num; i++)
      {
      cout << buf.getint();
      if (i+1<num) cout << ' ';
      }
    cout << dec << endl;
    }
  rest(buf);
  }
catch (C_error* e)
  {
  cout << endl << (*e) << endl; delete e;
  cout << "buffer: " << hex << buf << dec << endl;
  }
}

/*****************************************************************************/

void C_DoCommand_r_message::print()
{
//cout << "Server ist "<<server<<endl;
try
  {
  int pnum;
  cout << "  IS " << buf.getint() <<"; "<<(pnum=buf.getint())<<" procs"<< endl;
  for (int p=0; p<pnum; p++)
    {
    int proc=buf.getint();
    int num=buf.getint();
    cout << "  proc "<<proc<<"; ";
    if (num)
      cout<<num<<" args(hex): " << hex;
    else
      cout<<"no args";
    for (int i=0; i<num; i++)
      {
      cout << ' ' << buf.getint();
      if (i%10==9) cout << endl;
      }
    cout << dec << endl;
    }
  rest(buf);
  }
catch (C_error* e)
  {
  cout << endl << (*e) << endl; delete e;
  cout << "buffer: " << hex << buf << dec << endl;
  }
}

/*****************************************************************************/

void C_WindDataout_r_message::print()
{
cout << "  idx: " << buf.getint() << "; size: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_WriteDataout_r_message::print()
{
int idx, header, num, size, i, j;
size=buf.size();
buf >> idx >> header >> num;
cout << "  idx: "<<idx<<"; write header: "<<header<<"; "<<num<<" words"<<endl;
cout << "  message size: "<<size<<endl<<hex;
if (size<=50)
  {
  cout << "  all words(hex):"<<endl<<hex;
  for (i=0; i<size; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
else
  {
  cout << "  first words(hex):"<<endl<<hex;
  for (i=0; i<20; i++)
    {
    cout << ' ' << buf[i];
    if (i%10==9) cout << endl;
    }
  cout <<dec<<endl;
  cout << "  last words(hex):"<<endl<<hex;
  for (i=size-20, j=0; i<size; i++, j++)
    {
    cout << ' ' << buf[i];
    if (j%10==9) cout << endl;
    }
  cout <<dec<<endl;
  }
}

/*****************************************************************************/

void C_GetDataoutStatus_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_EnableDataout_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/

void C_DisableDataout_r_message::print()
{
cout << "  idx: " << buf.getint() << endl;
rest(buf);
}

/*****************************************************************************/
/*****************************************************************************/
