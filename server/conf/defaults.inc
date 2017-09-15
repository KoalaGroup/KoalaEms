/*
 * defaults.inc
 * 28.01.94
 */

isrodummys = DownloadReadoutList UploadReadoutList DeleteReadoutList EnableIS DisableIS GetISStatus
modlistdummies = DownloadISModulList UploadISModulList DeleteISModulList
isdummys = $(isrodummys) $(modlistdummies) CreateIS DeleteIS GetISParams
procdummys = TestCommand DoCommand
vardummys = CreateVariable DeleteVariable ReadVariable WriteVariable GetVariableAttributes
pidummys = CreateProgramInvocation DeleteProgramInvocation StartProgramInvocation ResetProgramInvocation StopProgramInvocation ResumeProgramInvocation GetProgramInvocationAttr GetProgramInvocationParams
dodummys = WindDataout WriteDataout GetDataoutStatus EnableDataout DisableDataout
domdummys = DownloadDomain UploadDomain DeleteDomain
immerdummys = ResetISStatus Nothing

#ifdef OSK
%define OPTIMIERT
#endif

%define OUT_MAX (16*1024)
/*
 * end of defaults.inc
 */
