# lists the modullist

proc list_dom_modlist {ved} {

puts "domain modullist:"
if [catch {set mlist [$ved modullist upload]} mist] {
  puts $mist
} else {
  if {$mlist==""} {
    puts " the modullist is empty"
  } else {
    foreach m $mlist {
      set slot [lindex $m 0]
      set type [lindex $m 1]
      set name [modultypename $type]
      puts [format {%2d 0x%08X   %s} $slot $type $name]
    }
  }
}
}
