proc extract_deadtime {list} {
  set sum [expr double([lindex $list 5])*4294967296+[lindex $list 6]]
  set n [lindex $list 1]
  if {$n>0} {
    set dt [expr $sum/$n/10000.]
  } else {
    set dt 0
  }
  return $dt
}

proc got_sync_statist {ved args} {
#zb: 16  1854349 0   2647419 1   1   0    0    -1  0   0 0 0 0    0 0 3000 0 0
#    len evcount rej rej     num typ entr Over Min Max Sum Sum**2 ...

  set num [lindex $args 4]
  set args [lrange $args 5 end]
  for {set i 0} {$i<$num} {incr i} {
    set type [lindex $args 0]
    set val($type) [extract_deadtime $args]
    set args [lrange $args 12 end]
  }
  
  if [info exists val(1)] {
    if [info exists val(2)] {
      set ::${ved}::dt [format {%.3f (%.3f) ms} $val(1) $val(2)]
    } else {
      set ::${ved}::dt [format {%.3f ms} $val(1)]
    }
  } else {
    set ::${ved}::dt "hä?"
  }
}

proc got_sync_statist_err {ved conf} {
  puts "got_sync_statist_err $ved: [$conf print]"
  $conf delete
}
