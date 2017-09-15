%define MAX_VAR 100
%define MAX_TRIGGER 16
%define MAX_IS 10
%define MAX_LAM 25

%define OBJ_PI
%define PI_READOUT
%define READOUT_CC
%define PI_LAM

%define OBJ_IS

%define OBJ_DOMAIN
%define DOM_ML
%define DOM_LAM
%define DOM_TRIGGER

%define PROCS
%define TRIGGER
%define LOWLEVEL

objects += VAR PI IS
domains += ml lam trigger
invocations += READOUT_CC LAM
trigger += CAMAC
procs += GENERAL CAMAC
lowlevel += CAMAC/VCC2117
