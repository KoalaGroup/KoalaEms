/*
 * emsc_message.cc.m4  
 * 
 * created: 18.03.95 PW
 * 29.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <emsc_message.hxx>
#include <conststrings.h>
#include <ved_errors.hxx>
#include "versions.hxx"

VERSION("Mar 27 2003", __FILE__, __DATE__, __TIME__,
"$ZEL: emsc_message.cc.m4,v 2.8 2004/11/26 22:20:38 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

void C_emsc_message::print()
{
C_EMS_message* msg;
Request type;

ved_versions->getversions(server, reqver, unsolver);
type=ved_versions->request(conf.header()->type.reqtype, reqver);
cout << Req_str(type);
if (reqver==-1) cout << " (?)";
cout << endl;
int err=buf.getint();
if (err)
  {
  cout << "  ERROR: ";
  C_ved_error* e=new C_ved_error((const C_VED*)0, &conf);
  cout << (*e) << endl;
  delete e;
  rest(buf);
  }
else
  {
  switch (type)
    {
  define(`version', `')
  define(`Req', `
  case Req_$1: msg=new C_$1_c_message(*this); break;
  ')
  include(COMMON_INCLUDE/requesttypes.inc)
    default: msg=new C_emsc_unknown_message(*this); break;
    }
  msg->print();
  delete msg;
  }
}

/*****************************************************************************/

void C_emsc_unknown_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Nichts_c_message::print()
{
rest(buf);
}

/*****************************************************************************/

void C_Initiate_c_message::print()
{
int VED_ver, requesttab_ver, unsolmsgtab_ver;
buf++; // errorcode
buf >> VED_ver >> requesttab_ver >> unsolmsgtab_ver;
cout << "  ver_VED: " << VED_ver
    << "; ver_requesttab: " << requesttab_ver
    << "; ver_unsolmsgtab: " << unsolmsgtab_ver << endl;
rest(buf);
}

/*****************************************************************************/

void C_Identify_c_message::print()
{
buf++; // errorcode
cout << "  ver_VED: " << buf.getint()
    << "; ver_requesttab: " << buf.getint()
    << "; ver_unsolmsgtab: " << buf.getint() << endl;
int num=buf.getint();
cout << "  " << num << " Strings:"<<endl;
for (int i=0; i<num; i++)
  {
  STRING s;
  buf >> s;
  cout << s << endl;
  }
rest(buf);
}

/*****************************************************************************/

void C_Conclude_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_ResetVED_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetVEDStatus_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetNameList_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetCapabilityList_c_message::print()
{
int num, i;
buf++;
buf >> num;
for (i=0; i<num; i++)
  {
  STRING s;
  int idx, ver;
  buf >> idx >> s >> ver;
  cout << "  " << setw(3) << idx << ": " << s << "." << ver << endl;
  }
rest(buf);
}

/*****************************************************************************/

void C_GetProcProperties_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DownloadDomain_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_UploadDomain_c_message::print()
{
buf++;
int num=buf.size()-1;
cout << "  " << num << " words:" << endl << " " << hex;
for (int i=0; i<num; i++) cout << ' ' << buf.getint();
cout << dec << endl;
rest(buf);
}

/*****************************************************************************/

void C_DeleteDomain_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_CreateProgramInvocation_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DeleteProgramInvocation_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_StartProgramInvocation_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_ResetProgramInvocation_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_ResumeProgramInvocation_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_StopProgramInvocation_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetProgramInvocationAttr_c_message::print()
{
buf++;
int status=buf.getint();
switch (status)
  {
  case -3: cout << "  error"; break;
  case -2: cout << "  no_more_data"; break;
  case -1: cout << "  stopped"; break;
  case 0: cout << "  inactive"; break;
  case 1: cout << "  running"; break;
  default: cout << "  status "<<status; break;
  }
cout << "; event " << buf.getint(); // eventcount
if (buf.size()>3)
  {
  double sec;
  int aux=buf.getint();
  switch (aux)
    {
    case 1:
      sec=buf.getint()+(double)buf.getint()/1000000.0;
      break;
    case 2:
      sec=86400*(buf.getint()-2440587)+buf.getint()+double(buf.getint())/100.0;
      break;
    default:
      sec=0;
      break;
    }
  cout << "; time: " << setprecision(20) << sec;
  }
cout << endl;
rest(buf);
}

/*****************************************************************************/

void C_GetProgramInvocationParams_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_CreateVariable_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DeleteVariable_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_ReadVariable_c_message::print()
{
buf++;
int num=buf.getint();
cout << "  " << num << " words(hex):" << endl
    << hex << setiosflags(ios::showbase);
for (int i=0; i<num; i++)
  {
  cout << ' ' << buf.getint();
  if (i%10==9) cout << endl;
  }
cout << dec << resetiosflags(ios::showbase) << endl;
rest(buf);
}

/*****************************************************************************/

void C_WriteVariable_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetVariableAttributes_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_CreateIS_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DeleteIS_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DownloadISModulList_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_UploadISModulList_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DeleteISModulList_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DownloadReadoutList_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_UploadReadoutList_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DeleteReadoutList_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_EnableIS_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DisableIS_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetISStatus_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetISParams_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_ResetISStatus_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_TestCommand_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DoCommand_c_message::print()
{
buf++;
int num=buf.size()-1;
cout << "  " << num << " words(hex):"<< endl << " " << hex;
for (int i=0; i<num; i++) cout << ' ' << buf.getint();
cout << dec << endl;
}

/*****************************************************************************/

void C_WindDataout_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_WriteDataout_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_GetDataoutStatus_c_message::print()
{
buf++;
int error, fertig, disabled;
buf >> error >> fertig >> disabled;
cout << "  error: " << error << "; status: ";
switch ((DoRunStatus)fertig)
  {
  case Do_running: cout << "running"; break;
  case Do_neverstarted: cout << "never_started"; break;
  case Do_done: cout << "done"; break;
  case Do_error: cout << "error"; break;
  default: cout << fertig; break;
  }
cout << "; switch: ";
switch ((DoSwitch)disabled)
  {
  case Do_noswitch: cout << "noswitch"; break;
  case Do_enabled: cout << "enabled"; break;
  case Do_disabled: cout << "disabled"; break;
  default: cout << disabled; break;
  }
cout << hex << setiosflags(ios::showbase);
while (!buf.empty())
  {
  cout << ' ' << buf.getint();
  }
cout << dec << resetiosflags(ios::showbase) << endl;
}

/*****************************************************************************/

void C_EnableDataout_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/

void C_DisableDataout_c_message::print()
{
buf++;
rest(buf);
}

/*****************************************************************************/
/*****************************************************************************/
