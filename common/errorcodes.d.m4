*******************************************************************************
*                                                                             *
* errorcodes.d.m4                                                             *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 08.10.93                                                                    *
*                                                                             *
*******************************************************************************

define(`e_code', `Err_$1 do.b 1')
define(`pl_code', `')
define(`rt_code',`')
  org 0
OK do.b 1
include(EMSDEFS/errorcodes.inc)

define(`e_code', `')
define(`pl_code', `plErr_$1 do.b 1')
define(`rt_code',`')
  org 0
plOK do.b 1
include(EMSDEFS/errorcodes.inc)

define(`e_code', `')
define(`pl_code',`')
define(`rt_code', `rtErr_$1 do.b 1')
  org 0
rtOK do.b 1
include(EMSDEFS/errorcodes.inc)

*******************************************************************************
*******************************************************************************

