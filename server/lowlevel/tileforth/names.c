/******************************************************************************
*                                                                             *
* names.c.m4                                                                  *
*                                                                             *
* OS9/ULTRIX                                                                  *
*                                                                             *
* created: 13.09.94                                                           *
* last changed: 13.09.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "names.h"

/*****************************************************************************/







char *errstr[]={
"OK",

"Err_IllRequest",
"Err_NotImpl",

"Err_ArgNum",
"Err_ArgRange",

"Err_IllObject",
"Err_ObjNonDel",
"Err_ObjDef",

"Err_IllVar",
"Err_NoVar",
"Err_VarDef",
"Err_IllVarSize",

"Err_IllDomain",
"Err_NoDomain",
"Err_DomainDef",
"Err_IllDomainType",

"Err_IllIS",
"Err_NoIS",
"Err_ISDef",
"Err_NoISModulList",
"Err_NoReadoutList",
"Err_IllTrigPatt",

"Err_IllPI",
"Err_NoPI",
"Err_PIDef",
"Err_IllPIType",
"Err_PIActive",
"Err_PINotActive",
"Err_NoTrigger",
"Err_NoSuchTrig",

"Err_IllCapTyp",
"Err_BadProcList",
"Err_ExecProcList",

"Err_TapeOpen_",
"Err_TapeNotOpen_",
"Err_DataoutBusy",
"Err_TapeEnabled_",
"Err_TapeNotEnabled_",

"Err_NoMem",
"Err_BufOverfl",
"Err_NoVED",
"Err_System",

"Err_Program",
"Err_IllDo",
"Err_NoDo",
"Err_TrigProc",






























};



char *reqstrs[]={

"Nothing",

"Initiate",
"Identify",
"Conclude",
"ResetVED",
"GetVEDStatus",
"GetNameList",
"GetCapabilityList",

"DownloadDomain",
"UploadDomain",
"DeleteDomain",

"CreateProgramInvocation",
"DeleteProgramInvocation",
"StartProgramInvocation",
"ResetProgramInvocation",
"ResumeProgramInvocation",
"StopProgramInvocation",
"GetProgramInvocationAttr",

"CreateVariable",
"DeleteVariable",
"ReadVariable",
"WriteVariable",
"GetVariableAttributes",

"CreateIS",
"DeleteIS",
"DownloadISModulList",
"UploadISModulList",
"DeleteISModulList",
"DownloadReadoutList",
"UploadReadoutList",
"DeleteReadoutList",
"EnableIS",
"DisableIS",
"GetISStatus",
"ResetISStatus",

"TestCommand",
"DoCommand",

"WindDataout",
"WriteDataout",
"GetDataoutStatus",
"EnableDataout",
"DisableDataout",

"ProfileVED",
"ProfileIS",
"ProfileDI",
"ProfileDO",
"ProfilePI",
"ProfileTR",



};

/*****************************************************************************/
/*****************************************************************************/
