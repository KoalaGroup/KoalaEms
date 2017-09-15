/******************************************************************************
*                                                                             *
* em_unix.inc                                                                 *
*                                                                             *
* 19.03.97                                                                    *
*                                                                             *
******************************************************************************/

#include "defaults.inc"

%define READOUT_PRIOR 100
%define MAX_DATAIN 20
%define MAX_DATAOUT 5
%define EVENT_MAX 204800

dummyprocs = $(isdummys) $(procdummys) $(vardummys) $(immerdummys)

objects = pi domain

#ifndef NODATAOUT
#include "emout_unix.inc"
#else
dataout = dummy
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

commu = socket
domains = event datain dataout
invocations = readout_em
lowlevel += oscompat
