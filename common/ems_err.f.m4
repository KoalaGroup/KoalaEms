C******************************************************************************
C                                                                             *
C ems_err.f.m4                                                                *
C                                                                             *
C 01.09.94                                                                    *
C                                                                             *
C******************************************************************************

define(start, 1000)
C     Fehlercodes fuer EMS_errno
      PARAMETER EMSE_OK            =0
      PARAMETER EMSE_Start         =start
define(emse_code, `define(`start', incr(start))'`      PARAMETER $1=start')
include(EMSDEFS/ems_err.inc)
      PARAMETER NrOfEMSErrs        =incr(start)

C******************************************************************************
C******************************************************************************
