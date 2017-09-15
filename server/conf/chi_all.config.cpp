


























isrodummys = DownloadReadoutList UploadReadoutList DeleteReadoutList EnableIS DisableIS GetISStatus
isdummys = $(isrodummys) CreateIS DeleteIS DownloadISModulList UploadISModulList DeleteISModulList GetISParams
procdummys = TestCommand DoCommand
vardummys = CreateVariable DeleteVariable ReadVariable WriteVariable GetVariableAttributes
pidummys = CreateProgramInvocation DeleteProgramInvocation StartProgramInvocation ResetProgramInvocation StopProgramInvocation ResumeProgramInvocation GetProgramInvocationAttr GetProgramInvocationParams
dodummys = WindDataout WriteDataout GetDataoutStatus EnableDataout DisableDataout
domdummys = DownloadDomain UploadDomain DeleteDomain
immerdummys = ResetISStatus Nothing
prodummys = ProfileVED ProfileIS ProfileDI ProfileDO ProfilePI ProfileTR

immerdummys += $(prodummys)




%define OUT_MAX (16*1024)

%undef OPTIMIERT

%define READOUT_PRIOR 100

%define EVENT_BUFBEG 0x20000000
%define EVENT_BUFEND (EVENT_BUFBEG+0x20000*4)
%define EVENT_MAX 15360

%define OUT_BUFBEG EVENT_BUFEND
%define OUT_BUFEND (OUT_BUFBEG+0x10000*4)

%define FB_BUFBEG OUT_BUFEND
%define FB_BUFEND (FB_BUFBEG+0x10000*4)

%define MAX_VAR 100
%define MAX_TRIGGER 16
%define MAX_IS 20

%define OBJ_PI
%define PI_READOUT
%define READOUT_CC

%define OBJ_IS

%define OBJ_DOMAIN
%define DOM_ML
%define DOM_TRIGGER
%define DOM_EVENT
%define DOM_DATAOUT

%define OBJ_VAR

%define PROCS
%define TRIGGER
%define LOWLEVEL


%define DATAOUT_SIMPLE
dataout = RINGBUFFER


%define DEFPORT 2048
commu = SOCKET
domains = ml trigger event dataout
objects = VAR PI DOMAIN IS
invocations = READOUT_CC
trigger = GENERAL
procs = GENERAL
lowlevel = CHI
dummyprocs = $(dodummys) $(immerdummys)

%undef READOUT_PRIOR
%define READOUT_PRIOR 1600

%define OPTIMIERT
%define TEST_ML

procs += TEST GENERAL/STRUCT GENERAL/VARS GENERAL/RAW CHI/TRIGGER FASTBUS/FB_CHIPS FASTBUS_N FASTBUS/FB_LECROY

trigger += CHI/GSI

lowlevel = BUSTRAP CHI_GSI CHI_NEU OSCOMPAT

extralflags = -g -Fl68s1000

extralibs += $(BERKLIB)



