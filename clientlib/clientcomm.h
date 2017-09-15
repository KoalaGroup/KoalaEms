/*
 * clientcomm.h
 *
 * $ZEL: clientcomm.h,v 2.13 2004/11/26 14:37:13 wuestner Exp $
 *
 * created before 15.08.94
 * 11.09.1998 PW: debugcl changed to policies
 * 22.03.1999 PW: GetDataoutStatus_1 added
 * 16.Apr.2004 PW: FlushDataout added
 */

#ifndef _clientcomm_h_
#define _clientcomm_h_

#include <sys/time.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <objecttypes.h>
#include <cdefs.h>
#include <ems_err.h>
#include <msg.h>
#include "ccommcallback.h"

extern EMSerrcode EMS_errno;
extern unsigned int client_id;
extern int def_unsol;
typedef enum
  {
  pol_none=0,
  pol_nodebug=1, /* do not generate debug messages */
  pol_noshow =2, /* do not show log messages */
  pol_nocount=4, /* do not count for autoexit */
  pol_nowait =8  /* do not wait for this station (client) at exit */
  } en_policies;
extern en_policies policies;

void free_confirmation(ems_i32 *conf);
int test_path(int path);
int getconfpath();
void settimeout(int sec);
int GetConfirmation(ems_u32* ved, union Reqtype* requesttype,
    ems_u32* transaction, ems_u32* size, ems_i32** confirmation,
    const struct timeval *timeout);
int GetConfir(ems_i32* xid, ems_u32* ved, union Reqtype* req,
    ems_i32** confirmation, msgheader* header, const struct timeval *timeout);
int GetConfirmation_h(ems_i32** confirmation, msgheader* header,
    const struct timeval *timeout);
int GetSpecialConfirmation(ems_u32* ved, union Reqtype* requesttype,
    ems_i32* transaction, ems_u32* size, ems_i32** confirmation,
    const struct timeval *timeout);
int GetSpecialConfirmation_h(ems_u32* ved, int* requesttype,
    ems_u32* transaction,
    ems_u32* size, ems_i32** confirmation, const struct timeval* timeout,
    msgheader* header);
int Clientcomm_init(const char* sockname);
int Clientcomm_init_e(const char* hostname, int port);
int Clientcomm_done();
void Clientcomm_abort(int);
int OpenVED(const char* ved_name);
int CloseVED(ems_u32 ved);
int GetHandle(const char *ved_name);
int GetName(ems_u32 ved, char* name);
int Initiate(ems_u32 ved, ems_u32 ved_id);
int Identify(ems_u32 ved, int mod);
int Conclude(ems_u32 ved);
int ResetVED(ems_u32 ved);
int GetVEDStatus(ems_u32 ved);
ems_i32 GetNameList(ems_u32 ved, int count, int *req);
ems_i32 GetCapabilityList(ems_u32 ved, Capabtyp type);
ems_i32 GetProcProperties(ems_u32 ved, ems_u32 level, ems_u32 num,
    const ems_u32* list);
ems_i32 DownloadDomain(ems_u32 ved, Domain type, ems_u32 domain_ID, int size,
    const ems_u32* domain);
ems_i32 UploadDomain(ems_u32 ved, Domain type, ems_u32 domain_ID);
ems_i32 DeleteDomain(ems_u32 ved, Domain type, ems_u32 domain_ID);
ems_i32 CreateProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID,
    int numparams, const ems_u32* params);
ems_i32 DeleteProgramInvocation(ems_u32 ved, Invocation type,
    ems_u32 invocation_ID);
ems_i32 StartProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID);
ems_i32 ResetProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID);
ems_i32 ResumeProgramInvocation(ems_u32 ved, Invocation type,
    ems_u32 invocation_ID);
ems_i32 StopProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID);
ems_i32 GetProgramInvocationAttributes(ems_u32 ved, Invocation type,
    ems_u32 invocation_ID);
ems_i32 GetProgramInvocationParams(ems_u32 ved, Invocation type,
    ems_u32 invocation_ID);
ems_i32 CreateVariable(ems_u32 ved, ems_u32 variable_ID, int size);
ems_i32 DeleteVariable(ems_u32 ved, ems_u32 variable_ID);
ems_i32 ReadVariable(ems_u32 ved, ems_u32 variable_ID);
ems_i32 WriteVariable(ems_u32 ved, ems_u32 variable_ID, ems_u32 count,
    const ems_u32* val);
ems_i32 GetVariableAttributes(ems_u32 ved, ems_u32 variable_ID);
ems_i32 CreateIS(ems_u32 ved, ems_u32 IS_ID, int ident);
ems_i32 DeleteIS(ems_u32 ved, ems_u32 IS_ID);
ems_i32 DownloadISModulList(ems_u32 ved, ems_u32 IS_ID, ems_u32 nummod,
    const ems_u32* list);
ems_i32 UploadISModulList(ems_u32 ved, ems_u32 IS_ID);
ems_i32 DeleteISModulList(ems_u32 ved, ems_u32 IS_ID);
ems_i32 DownloadReadoutList(ems_u32 ved, ems_u32 IS_ID, ems_u32 priority,
    ems_u32 numtrig, const ems_u32* trigger, int listsize, const ems_u32* list);
ems_i32 UploadReadoutList(ems_u32 ved, ems_u32 IS_ID, int trigger);
ems_i32 DeleteReadoutList(ems_u32 ved, ems_u32 IS_ID, int trigger);
ems_i32 EnableIS(ems_u32 ved, ems_u32 IS_ID);
ems_i32 DisableIS(ems_u32 ved, ems_u32 IS_ID);
ems_i32 GetISStatus(ems_u32 ved, ems_u32 IS_ID);
ems_i32 ResetISStatus(ems_u32 ved, ems_u32 IS_ID);
ems_i32 DoCommand(ems_u32 ved, ems_u32 IS_ID, int listsize, const ems_u32* list);
ems_i32 TestCommand(ems_u32 ved, ems_u32 IS_ID, int listsize, const ems_u32* list);
ems_i32 WindDataout(ems_u32 ved, ems_u32 DO_ID, ems_i32 offset);
ems_i32 WriteDataout(ems_u32 ved, ems_u32 DO_ID, ems_u32 makeheader, ems_u32 size,
    const ems_u32* record);
ems_i32 GetDataoutStatus(ems_u32 ved, ems_u32 DO_ID);
ems_i32 GetDataoutStatus_1(ems_u32 ved, ems_u32 DO_ID, int arg);
ems_i32 EnableDataout(ems_u32 ved, ems_u32 DO_ID);
ems_i32 DisableDataout(ems_u32 ved, ems_u32 DO_ID);
ems_i32 FlushDataout(ems_u32 ved, ems_u32 DO_ID);
ems_i32 ProfileVED(ems_u32 ved, int clear);
ems_i32 ProfileIS(ems_u32 ved, int ID, int clear);
ems_i32 ProfileDI(ems_u32 ved, int ID, int clear);
ems_i32 ProfileDO(ems_u32 ved, int ID, int clear);
ems_i32 ProfilePI(ems_u32 ved, int type, int ID, int clear);
ems_i32 ProfileTR(ems_u32 ved, int clear);
ems_i32 Rawrequest(ems_u32 ved, Request request, ems_u32 size, ems_u32 *body);
int ConfirmationAvailable();
int messagepending();
int SpecialConfirmationAvailable(ems_u32 ved, int requesttype, int transaction);
int SetExitCommu(ems_u32 val);
int SetUnsolicited(ems_u32 ved, ems_u32 val);
void send_info(const char *info);
void send_info1(const char *info);
int AttachVED(ems_u32 ved, int val);
int reqver(int);

#endif
/*****************************************************************************/
/*****************************************************************************/
