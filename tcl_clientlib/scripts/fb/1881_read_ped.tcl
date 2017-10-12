proc rped {ved slot} {

    # read csr0
    set v [$ved command1 frc 0 $slot 0]
    if {[lindex $v 0]} {
        puts "read CSR0: ss=[lindex $v 0]"
        return
    }
    set csr0 [lindex $v 1]
    puts "CSR0=[format {0x%08x} $csr0]"

    #read csr1
    set v [$ved command1 frc 0 $slot 1]
    if {[lindex $v 0]} {
        puts "read CSR1: ss=[lindex $v 0]"
        return
    }
    set csr1 [lindex $v 1]
    puts "CSR1=[format {0x%08x} $csr1]"

    # disable gate (reset)
    #$ved command1 fwc 0 $slot 1 [expr $csr0 & 0xfffffffb]
    $ved command1 fwc 0 $slot 0 0x40000000

    # read thresholds
    set a 0xc0000000
    for {set i 0} {$i<8} {incr i} {
        for {set j 0} {$j<8} {incr j} {
            set v [$ved command1 frc 0 $slot $a]
            if {[lindex $v 0]} {
                puts "read ped from $a: ss=[lindex $v 0]"
                return
            }
            set p [lindex $v 1]
            puts -nonewline "[format {%5d} $p] "
            incr a
        }
        puts {}
    }
    puts {}

    # reenable gate
    $ved command1 fwc 0 $slot 0 $csr0
}
