# $ZEL: strerror.tcl,v 1.4 2003/10/27 15:27:55 wuestner Exp $
# copyright 2000
#   P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc strerror {space errno} {
    ved_setup_$space eval set errno $errno
    set res [ved_setup_$space eval {
        if {![info exists uname]} {
            catch {set uname [ved command1 Uname | stringlist]}
        }
        if {![info exists errno_arr($errno)]} {
            catch {set errno_arr($errno) [ved command1 Strerror $errno | string]}
        }
        if [info exists errno_arr($errno)] {
            if [info exists uname] {
                set res "([lindex $uname 0]) "
            } else {
                set res ""
            }
            append res $errno_arr($errno)
        } else {
            set res "(bsd assumed) "
            append res [strerror_bsd $errno]
        }
        return $res
    }]
    return $res
}
