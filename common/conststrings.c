/*
 * common/conststrings.c
 * created before 22.03.95
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: conststrings.c,v 2.12 2013/01/17 22:37:47 wuestner Exp $";

#include <stdio.h>
#include <errno.h>
#include "ems_err.h"
#include "errorcodes.h"
#include "requesttypes.h"
#include "unsoltypes.h"
#include "intmsgtypes.h"
#include "objecttypes.h"
#include "conststrings.h"
#include "rcs_ids.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(HAVE_STRINGS_H) || defined(OSK)
#include <strings.h>
#else
#include <string.h>
#endif

extern char *emserrlstr[];

extern char *emserrstr[];
extern char *emserrlstr[];

extern char *errstr[];
extern char *errlstr[];
extern char *plerrstr[];
extern char *plerrlstr[];
extern char *rterrstr[];
extern char *rterrlstr[];

extern char *reqstrs[];
extern char *unsolstrs[];
extern char *intmsgstrs[];

static char errorstr[256];

#ifdef NEED_STRERROR
char *strerror __P((int error));
#endif

RCS_REGISTER(cvsid, "common")

/*****************************************************************************/
const char *EMS_errstr(EMSerrcode error)
{
if ((error>EMSE_Start) && ((unsigned int)error<NrOfEMSErrs))
  return(emserrlstr[error-EMSE_Start]);
else
{
  char *help;
  help=strerror(error);
  if(help)
    return(help);
  else{
    sprintf(errorstr,"unknown error (%d)",error);
    return(errorstr);
  }
}
}
/*****************************************************************************/
const char *R_errstr(errcode error)
{
if ((unsigned int)error<NrOfErrs)
  return(errlstr[error]);
sprintf(errorstr, "unknown Error (%d)", error);
return(errorstr);
}
/*****************************************************************************/
const char *PL_errstr(plerrcode error)
{
if ((unsigned int)error<NrOfplErrs)
  return(plerrlstr[error]);
sprintf(errorstr, "unknown Error (%d)", error);
return(errorstr);
}
/*****************************************************************************/
const char *RT_errstr(rterrcode error)
{
if ((unsigned int)error<NrOfrtErrs)
  return(rterrlstr[error]);
sprintf(errorstr, "unknown Error (%d)", error);
return(errorstr);
}
/*****************************************************************************/
const char* Req_str(Request nr)
{
if ((unsigned int)nr<NrOfReqs)
  return(reqstrs[nr]);
sprintf(errorstr, "unknown Req %d", nr);
return(errorstr);
}
/*****************************************************************************/
const char* Unsol_str(UnsolMsg nr)
{
if ((unsigned int)nr<NrOfUnsolmsg)
  return(unsolstrs[nr]);
sprintf(errorstr, "unknown Unsolmsg (%d)", nr);
return(errorstr);
}
/*****************************************************************************/
const char* Int_str(IntMsg nr)
{
if ((unsigned int)nr<NrOfIntmsg)
  return(intmsgstrs[nr]);
sprintf(errorstr, "unknown Intmsg (%d)", nr);
return(errorstr);
}
/*****************************************************************************/
const char* Object_str(Object nr)
{
switch (nr)
  {
  case Object_null: return("Null"); break;
  case Object_ved: return("VED"); break;
  case Object_domain: return("Domain"); break;
  case Object_is: return("IS"); break;
  case Object_var: return("Variable"); break;
  case Object_pi: return("ProgrammInvocations"); break;
  case Object_do: return("DataOut"); break;
  default:
    sprintf(errorstr, "unknown Object (%d)", nr);
    return(errorstr);
    break;
  }
return(0);
}
/*****************************************************************************/
const char* Domain_str(Domain nr)
{
switch (nr)
  {
  case Dom_null: return("Null"); break;
  case Dom_Modullist: return("Modullist"); break;
  case Dom_LAMproclist: return("LAMproclist"); break;
  case Dom_Trigger: return("Trigger"); break;
  case Dom_Event: return("Event"); break;
  case Dom_Datain: return("Datain"); break;
  case Dom_Dataout: return("Dataout"); break;
  default:
    sprintf(errorstr, "unknown Domain (%d)", nr);
    return(errorstr);
    break;
  }
return(0);
}
/*****************************************************************************/
const char* Invocation_str(Invocation nr)
{
switch (nr)
  {
  case Invocation_null: return("Null"); break;
  case Invocation_readout: return("Readout"); break;
  case Invocation_LAM: return("LAM"); break;
  default:
    sprintf(errorstr, "unknown Invocation (%d)", nr);
    return(errorstr);
    break;
  }
return(0);
}
/*****************************************************************************/
const char* Capabtyp_str(Capabtyp nr)
{
switch (nr)
  {
  case Capab_listproc: return("Listproc"); break;
  case Capab_trigproc: return("Trigproc"); break;
  default:
    sprintf(errorstr, "unknown Capabilitytyp (%d)", nr);
    return(errorstr);
    break;
  }
return(0);
}
/*****************************************************************************/
const char* InOutTyp_str(InOutTyp nr)
{
switch (nr)
  {
  case InOut_Ringbuffer: return("Ringbuffer"); break;
  default:
    sprintf(errorstr, "unknown InOutTyp (%d)", nr);
    return(errorstr);
    break;
  }
return(0);
}
/*****************************************************************************/
const char* IOAddr_str(IOAddr nr)
{
switch (nr)
  {
  case Addr_Raw: return("Raw"); break;
  case Addr_Modul: return("Modul"); break;
  case Addr_Driver_mapped: return("Driver_mapped"); break;
  case Addr_Driver_mixed: return("Driver_mixed"); break;
  case Addr_Driver_syscall: return("Driver_syscall"); break;
  case Addr_Socket: return("Socket"); break;
  case Addr_LocalSocket: return("LocalSocket"); break;
  case Addr_File: return("File"); break;
  case Addr_Tape: return("Tape"); break;
  default:
    sprintf(errorstr, "unknown IOAddresstype (%d)", nr);
    return(errorstr);
    break;
  }
return(0);
}
/*****************************************************************************/
const char* DataoutStatus_str(int nr)
{
switch (nr)
  {
  case 0: return("running"); break;
  case 1: return("not started"); break;
  case 2: return("finished"); break;
  case 3: return("finished with error"); break;
  default:
    sprintf(errorstr, "unknown state %d", nr);
    return(errorstr);
    break;
  }
return(0);
}
/*****************************************************************************/
/*****************************************************************************/
