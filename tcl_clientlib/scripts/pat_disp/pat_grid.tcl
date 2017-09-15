# $ZEL: pat_grid.tcl,v 1.1 2002/09/26 12:15:12 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc create_grid {frame} {
    global abs speed

    label $frame.h_bit   -text bit -anchor w
    #label $frame.h_name  -text name -anchor w
    label $frame.h_abs   -text {hits} -anchor w
    label $frame.h_speed -text {hits/s} -anchor w
    grid $frame.h_bit $frame.h_abs $frame.h_speed
    for {set i 0} {$i<16} {incr i} {
        set speed($i) 0
        set abs($i) 0
        label $frame.${i}bit -text $i -anchor e -relief ridge
        #entry $frame.${i}name -state disabled -textvariable 
        entry $frame.${i}abs -state disabled  -textvariable abs($i)\
            -relief ridge -justify right
        entry $frame.${i}speed -state disabled -textvariable speed($i)\
            -relief ridge -justify right
        grid $frame.${i}bit $frame.${i}abs $frame.${i}speed -sticky ew
    }
}
