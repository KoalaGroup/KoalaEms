/******************************************************************************
*                                                                             *
* chi.inc                                                                     *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 14.06.94                                                                    *
*                                                                             *
******************************************************************************/
#include "defaults.inc"
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

#ifndef NODATAOUT
%define DATAOUT_SIMPLE
dataout = RINGBUFFER
#else
dataout = DUMMY
#endif

%define DEFPORT 2048
commu = SOCKET
domains = ml trigger event dataout
objects = VAR PI DOMAIN IS
invocations = READOUT_CC
trigger = GENERAL
procs = GENERAL
lowlevel = CHI
dummyprocs = $(dodummys) $(immerdummys)
