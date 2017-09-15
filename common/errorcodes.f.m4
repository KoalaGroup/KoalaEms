C******************************************************************************
C                                                                             *
C errorcodes.f.m4                                                             *
C                                                                             *
C 07.12.93                                                                    *
C                                                                             *
C******************************************************************************

define(start, 0)
      PARAMETER OK=start
define(`e_code', `define(`start', incr(start))'`      PARAMETER Err_$1=start')
define(`pl_code', `')
define(`rt_code', `')
include(EMSDEFS/errorcodes.inc)
      PARAMETER NrOfErrs=incr(start)

define(`start', 0)
      PARAMETER plOK=start
define(`e_code', `')
define(`pl_code', `define(`start', incr(start))'`      PARAMETER plErr_$1=start')
define(`rt_code', `')
include(EMSDEFS/errorcodes.inc)
      PARAMETER NrOfplErrs=incr(start)

define(`start', 0)
      PARAMETER rtOK=start
define(`e_code', `')
define(`pl_code', `')
define(`rt_code', `define(`start', incr(start))'`      PARAMETER rtErr_$1=start')
include(EMSDEFS/errorcodes.inc)
      PARAMETER NrOfrtErrs=incr(start)

C******************************************************************************
C******************************************************************************

