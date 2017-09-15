# ems/common/requestarr.tcl.m4
# $ZEL: requestarr.tcl.m4,v 2.1 2009/08/10 14:33:43 wuestner Exp $
# 2009-08-10 PW

define(`version', `')

array set reqtypearr {
define(`i', 0)
define(`Req', `    i $1 define(`i', incr(i))')
include(EMSDEFS/requesttypes.inc)
}

array set reqnamearr {
define(`i', 0)
define(`Req', `    $1 i define(`i', incr(i))')
include(EMSDEFS/requesttypes.inc)
}
