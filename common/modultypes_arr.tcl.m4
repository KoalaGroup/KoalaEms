# ems/common/modultypes_arr.tcl.m4
# $ZEL: modultypes_arr.tcl.m4,v 2.2 2007/03/31 13:55:35 wuestner Exp $
# 2007-03-31 PW

define(`laenge', `ifelse(index($1, ` '), -1, len($1), index($1, ` '))')
define(`cut', `substr($1, 0, laenge($1))')
define(`module',`set modtypearr(cut($1)) $2')
include(EMSDEFS/modultypes.inc)

foreach i [array names modtypearr] {
    set modtypearr($i) [`format' {0x%08x} $modtypearr($i)]
}
foreach i [array names modtypearr] {
    set modnamearr($modtypearr($i)) $i
}
