/*
 * pcicamacstreamnewsync.inc
 * 10.07.1997
 */

%define NOLAMPOLL

#include "defaults.inc"
%define READOUT_PRIOR 1000
%define EVENT_MAX 20480
%define DEBUG
%define DEFPORT 2048

%define OBJ_VAR
%define MAX_VAR 100
%define ISVARS
%define MAX_TRIGGER 16
%define MAX_IS 10

%undef  OUT_MAX
%define OUT_MAX 1048576

%define OBJ_PI
%define PI_READOUT
%define READOUT_CC

%define OBJ_IS

%define OBJ_DOMAIN
%define DOM_ML
%define DOM_TRIGGER
%define DOM_EVENT
%define DOM_DATAOUT
%define DATAOUT_MULTI
%define MAX_DATAOUT 1

%define PROCS
%define TRIGGER
%define LOWLEVEL
extralflags = -g -lm
/*
 * extracflags = -Wreturn-type -Wunused -Wcomment -Wformat -Wparentheses -Wpointer-arith -Wconversion
 */

trigger = general camac camac/gsi camac/fsc pci/zelsync1
procs = general general/struct general/vars test internals/sched camac camac/fera camac/tdc camac/c219 camac/ral11 camac/scaler camac/caennet camac/trigger pci/trigger
dummyprocs = $(dodummys)  $(immerdummys)
lowlevel = oscompat sync/pci_zel1 camac/pcicamac

objects = var pi domain is
invocations = readout_cc
domains = ml trigger event dataout
commu = socket

%define PCICAMAC
%define PCICAM_MMAPPED
%define DRIVERCAMAC
