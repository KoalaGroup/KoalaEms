/* alle Readoutfunktionen, die unter MSDOS laufen */

#include "defaults.inc"

%define READOUT_PRIOR 100

%define EVENT_MAX 1024

%define DEBUG
%define DEFPORT 2048

commu = wattcp

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

%define PROCS
%define TRIGGER

objects = var pi domain is
invocations = readout_cc
domains = ml trigger event
trigger = general
procs = general general/struct general/vars test test/console
dataout = dummy

dummyprocs = $(dodummys)  $(immerdummys) $(prodummys)

