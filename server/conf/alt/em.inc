/******************************************************************************
*                                                                             *
* em.inc                                                                      *
*                                                                             *
* 28.01.94                                                                    *
*                                                                             *
******************************************************************************/

#include "defaults.inc"

%define READOUT_PRIOR 100
%define MAX_DATAIN 5
%define MAX_DATAOUT 5
%define EVENT_MAX 10240

dummyprocs = $(isdummys) $(procdummys) $(vardummys) $(immerdummys)

objects = PI DOMAIN

#ifndef NODATAOUT
#include "emout.inc"
#else
dataout = DUMMY
dummyprocs += $(dodummys)
#endif

%define DEFPORT 2048

%define OBJ_DOMAIN
%define DOM_DATAIN
%define DOM_DATAOUT
%define DOM_EVENT

%define OBJ_PI
%define PI_READOUT
%define NOLAMPOLL

%define LOWLEVEL

commu = SOCKET
domains = event datain dataout
invocations = READOUT_EM
lowlevel += OSCOMPAT
