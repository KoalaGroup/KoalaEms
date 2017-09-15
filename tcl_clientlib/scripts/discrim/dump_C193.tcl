proc dump_C193 {slot file} {
  global global_ved
  set slot1 [expr $slot+1]

  puts $file "CAEN C193 slot $slot"

  for {set i 0} {$i<16} {incr i} {
    set res [$global_ved command1 nAFread $slot $i 0]
    puts $file [format {%2d: %3d} $i [expr $res&0xff]]
  }
  for {set i 0} {$i<16} {incr i} {
    set res [$global_ved command1 nAFread $slot1 $i 0]
    puts $file [format {%2d: %3d} [expr $i+16] [expr $res&0xff]]
  }
  puts $file ""
}
