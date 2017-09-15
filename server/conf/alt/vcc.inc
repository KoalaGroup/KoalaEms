/******************************************************************************
*                                                                             *
* vcc.inc                                                                     *
*                                                                             *
* 28.01.94                                                                    *
*                                                                             *
******************************************************************************/

#include "defaults.inc"

%define READOUT_PRIOR 100

#include "vccin.inc"
#include "vccout.inc"

%define DOM_EVENT
%define DOM_DATAOUT

%define EVENT_MAX 1024

%define OBJ_VAR

#ifndef NODATAOUT
%define DATAOUT_SIMPLE
dataout = RINGBUFFER
#else
dataout = DUMMY
#endif

commu = PIPE
domains += event dataout
objects += DOMAIN
dummyprocs = $(dodummys) $(immerdummys)

