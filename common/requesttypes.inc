dnl
dnl common/requesttypes.inc
dnl
dnl $ZEL: requesttypes.inc,v 2.3 2009/08/10 14:36:42 wuestner Exp $
dnl
dnl created 02.08.94
dnl

Req(Nothing)

Req(Initiate)
Req(Identify)
Req(Conclude)
Req(ResetVED)
Req(GetVEDStatus)
Req(GetNameList)
Req(GetCapabilityList)
Req(GetProcProperties)

Req(DownloadDomain)
Req(UploadDomain)
Req(DeleteDomain)

Req(CreateProgramInvocation)
Req(DeleteProgramInvocation)
Req(StartProgramInvocation)
Req(ResetProgramInvocation)
Req(ResumeProgramInvocation)
Req(StopProgramInvocation)
Req(GetProgramInvocationAttr)
Req(GetProgramInvocationParams)

Req(CreateVariable)
Req(DeleteVariable)
Req(ReadVariable)
Req(WriteVariable)
Req(GetVariableAttributes)

Req(CreateIS)
Req(DeleteIS)
Req(DownloadISModulList)
Req(UploadISModulList)
Req(DeleteISModulList)
Req(DownloadReadoutList)
Req(UploadReadoutList)
Req(DeleteReadoutList)
Req(EnableIS)
Req(DisableIS)
Req(GetISStatus)
Req(GetISParams)
Req(ResetISStatus)

Req(TestCommand)
Req(DoCommand)

Req(WindDataout)
Req(WriteDataout)
Req(GetDataoutStatus)
Req(EnableDataout)
Req(DisableDataout)

version(2)

dnl ***************************************************************************
dnl ***************************************************************************
