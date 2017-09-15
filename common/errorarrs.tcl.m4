array set ems_errcodes {
define(`i', 1)
define(`e_code', `    i {$2} define(`i', incr(i))')
define(`pl_code', `')
define(`rt_code',`')
    0 OK
include(EMSDEFS/errorcodes.inc)
}

array set ems_plcodes {
define(`i', 1)
define(`e_code', `')
define(`pl_code', `    i {$2} define(`i', incr(i))')
define(`rt_code', `')
    0 plOK
include(EMSDEFS/errorcodes.inc)
}

array set ems_rtcodes {
define(`i', 1)
define(`e_code', `')
define(`pl_code', `')
define(`rt_code', `    i {$2} define(`i', incr(i))')
    0 rtOK
include(EMSDEFS/errorcodes.inc)
}
