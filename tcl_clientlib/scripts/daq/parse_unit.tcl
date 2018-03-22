# $ZEL: parse_unit.tcl,v 1.1 2017/11/08 12:58:19 wuestner Exp $
# copyright 2017
#   Peter Wuestner;
#       Forschungszentrum Juelich
#           Central Institute of Engineering, Electronics and Analytics
#               Electronic Systems (ZEA-2)

proc parse_unit_size {txt} {

    # remove leading and trailing spaces
    set rtxt [string trim $txt]

    set len [string length $rtxt]

    # an empty string is considered '1' or one Byte
    if {$len==0} {
        return 1
    }

    # txt has to end with a 'B' for Byte
    if {![string equal [string index $rtxt end] B]} {
        output "cannot parse unit string $txt" tag_red
        return 0
    }

    # trivial case: txt==B
    if {$len==1} {
        return 1;
    }
    
    # remove the B
    set rtxt [string range $rtxt 0 end-1]

    # next character may be an 'i' for binary prefix
    set binary 0
    if {[string equal [string index $rtxt end] i]} {
        set binary 1
        set rtxt [string range $rtxt 0 end-1]
    }

    # now hopefully one character only is left
    if {[string length $rtxt]!=1} {
        output "cannot parse unit string $txt" tag_red
        return 0
    }

    set pot [string first $rtxt {KkMGTPEZY}]
    if {$pot<0} {
        output "cannot parse unit string $txt" tag_red
        return 0
    }
    if {$pot==0} {
        set pot 1
    }

    if {$binary} {
        set unit [expr {2**($pot*10)}]
    } else {
        set unit [expr {10**($pot*3)}]
    }

    return $unit
}

proc parse_unit_time {txt} {
    
    # remove leading and trailing spaces
    set rtxt [string trim $txt]

    array set times {s 1 m 60 h 3600 d 86400 w 604800}

    if {![info exists times($rtxt)]} {
        output "cannot parse unit string $txt" tag_red
        return 0
    }

    set unit $times($rtxt)

    return $unit
}

proc parse_unit_rate {txt} {

    # we expect a form of size/time
    set list [split $txt {/}]

    if {[llength $list]!=2} {
        output "cannot parse unit string $txt" tag_red
        return 0
    }

    set sizeunit [parse_unit_size [lindex $list 0]]
    set timeunit [parse_unit_time [lindex $list 1]]

    if {$sizeunit==0 || $timeunit==0} {
        return 0
    }

    set unit [expr double($sizeunit)/$timeunit]

    return $unit
}
