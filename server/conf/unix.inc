/* alle Readoutfunktionen, die unter ULTRIX und OSF/1 laufen */

#include "defaults.inc"

%define READOUT_PRIOR 100

%define EVENT_MAX 1024

%define DEBUG
%define DEFPORT 2048

#ifdef GO32
commu = wattcp
#else
commu = socket
#endif
%define COMMU_ACCESSCONTROL

%define OBJ_VAR
%define MAX_VAR 20

%define OBJ_PI
%define PI_READOUT
%define READOUT_CC

%define OBJ_IS
%define MAX_IS 10

%define OBJ_DOMAIN
%define DOM_TRIGGER
%define MAX_TRIGGER 5
%define DOM_EVENT
%define DOM_ML
%define DOM_DATAOUT

%define DATAOUT_MULTI
%define MAX_DATAOUT 1

%define PROCS
%define TRIGGER

objects = var pi domain is
invocations = readout_cc
domains = ml trigger event dataout
trigger = general
procs = general general/struct general/vars test test/console internals/sched
dataout = stream/socket

#ifdef Lynx
%define EVENT_BUFBEG 0x880000
%define EVENT_BUFSIZE 0x80000
#else
%define EVENTBUFNAME "serverout"
/*
%define EVENT_BUFBEG 0
%define EVENT_BUFSIZE 50000
*/
#endif

dummyprocs = $(dodummys)  $(immerdummys)

%define LOWLEVEL
lowlevel = oscompat

#ifdef CAMAC
%define PI_LAM
%define MAX_LAM 25
%define DOM_LAM
#ifdef Lynx
%define DIRECTCAMAC
#else
%define DRIVERCAMAC
#endif
invocations += lam
domains += lam
trigger += camac camac/gsi camac/fsc
procs += camac camac/fera camac/tdc
lowlevel += camac/vcc2117
#endif

#ifdef VICBUS
procs += test/vicraw
procs += diskriminator
lowlevel += vicvsb
#endif

#ifdef FORTH
procs += tileforth
lowlevel += tileforth
#endif
