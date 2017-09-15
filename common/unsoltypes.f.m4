C******************************************************************************
C                                                                             *
C unsoltypes.f.m4                                                             *
C                                                                             *
C 15.02.93                                                                    *
C                                                                             *
C******************************************************************************

define(start, -1)
define(`Unsol', `define(`start', incr(start))'`      PARAMETER Unsol_$1=start')
define(`version', `')
include(EMSDEFS/unsoltypes.inc)
      PARAMETER NrOfUnsolmsg=incr(start)
define(`Unsol', `')
define(`version', `      PARAMETER ver_unsolmsgtab=$1')
include(EMSDEFS/unsoltypes.inc)

C******************************************************************************
C******************************************************************************

