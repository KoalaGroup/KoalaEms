/*
 * commu_local_server_a.cc
 * 
 * created 07.12.94 PW
 * 
 */

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <commu_local_server_a.hxx>
#include <objecttypes.h>
#include <xdrstring.h>
#include <xdrstrdup.hxx>
#include <commu_log.hxx>
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_local_server_a.cc,v 2.17 2014/07/14 15:12:19 wuestner Exp $")
#define XVERSION

extern C_log elog, nlog, dlog;
extern int leavemainloop;
extern int leaveimmediatly;
extern int autoexit;
extern int allowautoexit;
#ifdef COSYLOG
extern C_cosylogger* lgg_col;
#endif

using namespace std;

/*****************************************************************************/

C_local_server_a::C_local_server_a(const char *name, en_policies policy)
:C_local_server(name, policy), addr(name)
{
TR(C_local_server_a::C_local_server_a)
}

/*****************************************************************************/

C_local_server_a::~C_local_server_a()
{
TR(C_local_server_a::~C_local_dserver_c)
}

/*****************************************************************************/

errcode C_local_server_a::Nothing(const ems_u32* ptr, int num)
{
return(OK);
}

/*****************************************************************************/

errcode C_local_server_a::Initiate(const ems_u32* ptr, int num)
{
if (num!=1) return(Err_ArgNum);
// ptr[0] (ved_id) wird ignoriert
outbuf << ver_ved << ver_req << ver_unsol;
return(OK);
}

/*****************************************************************************/

errcode C_local_server_a::Conclude(const ems_u32* ptr, int num)
{
if (num!=0) return(Err_ArgNum);
delete this;
return(OK);
}

/*****************************************************************************/

errcode C_local_server_a::Identify(const ems_u32* ptr, int num)
{
int x, n;
char s[64];

if (num!=1) return(Err_ArgNum);
if ((unsigned)ptr[0]>2) return(Err_ArgRange);
outbuf << ver_ved << ver_req << ver_unsol << space_(x);
n=0;
if (ptr[0]>0)
  {
  outbuf << "commu"; n++;
  outbuf << strcat(strcat(strcat(s, __DATE__), "; "), __TIME__); n++;
  }
if (ptr[0]>1)
  {
  outbuf << "Configuration folgt spaeter";
  n++;
  }
outbuf << put(x, n);
return(OK);
}

/*****************************************************************************/

errcode C_local_server_a::GetNameList(const ems_u32* ptr, int num)
{
if (num==0) /* Baum aller Objekte gefordert */
  {
  return(Err_ArgNum);
  }
else
  {
  switch ((Object)ptr[0])
    {
    case Object_null:
      {
      if (num>1) return(Err_ArgNum);
      outbuf << 2; // Anzahl
      outbuf << Object_ved;
      outbuf << Object_is;
      return(OK);
      }
    case Object_ved:
      if (num>1) return(Err_ArgNum);
      outbuf << 1;
      outbuf << 0;
      return(OK);
    case Object_is:
      if (num>1) return(Err_ArgNum);
      outbuf << 0;
      return(OK);
    default :  return(Err_IllObject);
    }
  }
}

/*****************************************************************************/

errcode C_local_server_a::GetCapabilityList(const ems_u32* ptr, int num)
{
int i;

if (num!=1) return(Err_ArgNum);
if (ptr[0]!=Capab_listproc) return(Err_IllCapTyp);

outbuf << NrOfProcs;
for (i=0; i<NrOfProcs; i++)
  {
  outbuf << i;
  outbuf << Proc[i].name_proc;
  outbuf << *(Proc[i].ver_proc);
  }
return(OK);
}

/*****************************************************************************/
errcode C_local_server_a::GetProcProperties(const ems_u32* ptr, int num)
{
    unsigned int i;

    for (i=0; i<ptr[1]; i++) {
        if ((int)ptr[i+2]>=NrOfProcs) {
            outbuf << ptr[i+2];
            return(Err_ArgRange);
        }
    }
    outbuf << ptr[0];
    outbuf << ptr[1];
    for (i=0; i<ptr[1]; i++) {
        int proc_id;
        procprop* prop;

        proc_id=ptr[i+2];
        outbuf << proc_id;
        prop=(this->*Prop[proc_id].prop_proc)();
        outbuf << prop->varlength;  /* (0: constant, 1: variable) */
        outbuf << prop->maxlength;
        if (ptr[0]>0) {
            if (prop->syntax)
                outbuf <<prop->syntax;
            else
                outbuf << "";
        }
        if (ptr[0]>1) {
            if (prop->text)
                outbuf << prop->text;
            else
                outbuf << "";
        }
    }
    return(OK);
}
/*****************************************************************************/

errcode C_local_server_a::DoCommand(const ems_u32* ptr, int len)
{
errcode res;

if (len<2) return(Err_ArgNum);
if (ptr[0]!=0) return(Err_NoIS);

if ((res=test_proclist(ptr+1, len))!=OK) return(res);
return(scan_proclist(ptr+1));
}

/*****************************************************************************/

errcode C_local_server_a::TestCommand(const ems_u32* ptr, int len)
{
if (len<2) return(Err_ArgNum);
if (ptr[0]!=0) return(Err_NoIS);

return(test_proclist(ptr+1, len));
}

/*****************************************************************************/

errcode C_local_server_a::test_proclist(const ems_u32* ptr, int len)
{
plerrcode res;
const ems_u32* max;
int anz, i, x1, x2;

res=plOK;
max=ptr+len;
anz= *ptr++;
for (i=0; i<anz; i++)
  {
  outbuf<<space_(x1)<<space_(x2)<<space_for_counter();
  if ((int)(*ptr)>=NrOfProcs)
    {
    outbuf << *ptr;
    res=plErr_NoSuchProc;
    }
  else
    {
    if (ptr+*(ptr+1)+2>max)
      {
      outbuf << *(ptr+1);
      res=plErr_BadArgCounter;
      }
    else
      {
      res=(this->*Proc[*ptr].test_proc)(ptr+1);
      }
    }
  if (res)
    {
    outbuf<<put(x1, i+1)<<put(x2, res)<<set_counter();
    return(Err_BadProcList);
    }
  else
    {
    outbuf.index(x1);
    }
  ptr+= *(ptr+1)+2;
  }
return(OK);
}

/*****************************************************************************/

errcode C_local_server_a::scan_proclist(const ems_u32* ptr)
{
int i, num;
plerrcode res;

i=num= *ptr++;
while (i--)
  {
  if ((res=(this->*Proc[*ptr].proc)(ptr+1)))
    {
    outbuf << num-i << res;
    return(Err_ExecProcList);
    }
  ptr+= *(ptr+1)+2;
  }
return(OK);
}

/*****************************************************************************/
/*****************************************************************************/

plerrcode C_local_server_a::proc_echo(const ems_u32* ptr)
{
int i, size;

size=ptr[0];
for (i=1; i<=size; i++) outbuf << ptr[i];
return(plOK);
}

plerrcode C_local_server_a::test_proc_echo(const ems_u32* ptr)
{
return(plOK);
}

C_local_server::procprop* C_local_server_a::prop_proc_echo()
{
static procprop prop={1, -1, 0, "schickt alles zurueck"};
return(&prop);
}

const char C_local_server_a::name_proc_echo[]="Echo";
int C_local_server_a::ver_proc_echo=1;

/*****************************************************************************/
// addlogwin
// ptr[0]: size
// ptr[1...]: Display
//
#ifdef COSYLOG
plerrcode C_local_server_a::proc_addlogwin(const ems_u32* ptr)
{
char* disp;

disp=xdrstrdup(ptr+1);
if (lgg_col) outbuf << lgg_col->add_window(disp);
delete disp;
return(plOK);
}

plerrcode C_local_server_a::test_proc_addlogwin(const ems_u32* ptr)
{
if (ptr[0]<1) return(plErr_ArgNum);
if (ptr[0]!=xdrstrlen(ptr+1)) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_a::prop_proc_addlogwin()
{
static procprop prop={0, 1, "Display", "oeffnet neues CosyLog-Window"};
return(&prop);
}

const char C_local_server_a::name_proc_addlogwin[]="AddLogWin";
int C_local_server_a::ver_proc_addlogwin=1;
#endif
/*****************************************************************************/
// joinlog
// ptr[0]: size (==0)
// (spaeter: ptr[1...]: socket)
//
#ifdef COSYLOG
plerrcode C_local_server_a::proc_joinlog(const ems_u32* ptr)
{
if (lgg_col) outbuf << lgg_col->retry();
return(plOK);
}

plerrcode C_local_server_a::test_proc_joinlog(const ems_u32* ptr)
{
if (ptr[0]!=0) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_a::prop_proc_joinlog()
{
static procprop prop={0, 0, "void", "lenkt logg zu CosyLog um"};
return(&prop);
}

const char C_local_server_a::name_proc_joinlog[]="JoinLog";
int C_local_server_a::ver_proc_joinlog=1;
#endif
/*****************************************************************************/
// exit
// ptr[0]: size (==1)
// ptr[1]: immediate
//
plerrcode C_local_server_a::proc_exit(const ems_u32* ptr)
{
leavemainloop=1;
if (ptr[1]) leaveimmediatly=1;
return(plOK);
}

plerrcode C_local_server_a::test_proc_exit(const ems_u32* ptr)
{
if (ptr[0]!=1) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_a::prop_proc_exit()
{
static procprop prop={0, 0, "int", "beendet commu"};
return(&prop);
}

const char C_local_server_a::name_proc_exit[]="Exit";
int C_local_server_a::ver_proc_exit=1;

/*****************************************************************************/
// autoexit
// ptr[0]: size (==1)
// ptr[1]: -1: ask; 0: off; 1: on
//
plerrcode C_local_server_a::proc_autoexit(const ems_u32* ptr)
{
    outbuf << autoexit;
    int ptr1=(int)ptr[1];

    if ((ptr1>1) || (ptr1<-1))
        return(plErr_ArgRange);

    if (ptr1!=-1) {
        if (allowautoexit) {
            autoexit=ptr1;
            nlog << time << "set autoexit " << (autoexit?"on":"off") << flush;
        } else {
            nlog << time << "set autoexit " << (ptr1?"on":"off") << " denied"
                << flush;
        }
    }
    return plOK;
}

plerrcode C_local_server_a::test_proc_autoexit(const ems_u32* ptr)
{
if (ptr[0]!=1) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_a::prop_proc_autoexit()
{
static procprop prop={0, 0, "int", 0};
return(&prop);
}

const char C_local_server_a::name_proc_autoexit[]="AutoExit";
int C_local_server_a::ver_proc_autoexit=1;

/*****************************************************************************/
// exitlaw
// ptr[0]: size (==1)
// ptr[1]: allow
//
plerrcode C_local_server_a::proc_exitlaw(const ems_u32* ptr)
{
outbuf << allowautoexit;
if (ptr[1]==0)
  {
  allowautoexit=0;
  autoexit=0;
  }
else
  allowautoexit=1;
return(plOK);
}

plerrcode C_local_server_a::test_proc_exitlaw(const ems_u32* ptr)
{
if (ptr[0]!=1) return(plErr_ArgNum);
return(plOK);
}

C_local_server::procprop* C_local_server_a::prop_proc_exitlaw()
{
static procprop prop={0, 0, "allow", "erlaubt oder verbietet autoexit"};
return(&prop);
}

const char C_local_server_a::name_proc_exitlaw[]="ExitLaw";
int C_local_server_a::ver_proc_exitlaw=1;

/*****************************************************************************/
/*****************************************************************************/
