proc dump_4413 {slot file} {
  global global_ved

  set mask [expr [$global_ved command1 nAFread $slot 0 0]&0xffff]
  set thresh [expr [$global_ved command1 nAFread $slot 0 1]&0x3ff]

  puts $file "LeCroy 4413 slot $slot"
  puts $file "threshold=$thresh"
  puts $file "mask     =[format {0x%04x} $mask]"
  puts $file ""
}
