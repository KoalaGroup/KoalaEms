/******************************************************************************
*                                                                             *
* chimaere.inc                                                                *
*                                                                             *
* 16.03.94                                                                    *
*                                                                             *
******************************************************************************/

#include "defaults.inc"

%define READOUT_PRIOR 100
%define MAX_DATAOUT 5

%define EVENT_MAX 10240
%define MAX_VAR 100
%define MAX_TRIGGER 16
%define MAX_IS 10

%define OBJ_PI
%define PI_READOUT
%define READOUT_CC
%define NOLAMPOLL

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

dummyprocs = $(immerdummys)

objects = PI DOMAIN VAR IS

#include "emout.inc"

%define DEFPORT 2048

commu = SOCKET
domains = event dataout ml trigger
invocations= READOUT_CC
trigger = GENERAL
procs = GENERAL TEST
lowlevel = OSCOMPAT

extralibs += $(BERKLIB)
