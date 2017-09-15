# $ZEL: calculate_intsize.tcl,v 1.4 2010/09/09 23:17:14 wuestner Exp $
# copyright: 1998, 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc calculate_intsize {} {
    global tcl_platform

    if {[info exists tcl_platform(wordSize)]} {
        set bits [expr $tcl_platform(wordSize)*8]
        puts "wordSize: $bits bits"
        return $bits
    }

    for {set bits 1} {$bits<=128} {incr bits} {
        set val 1
        for {set i 1} {$i<$bits} {incr i} {
            set val [expr $val*2]
        }
        if {$val<0} {
            puts "bits=$bits"
            if {$bits>64} {
                set bits 64
            }
            return $bits
        }
    }
    return 0
}
