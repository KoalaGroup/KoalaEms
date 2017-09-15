proc got_do_list {ved args} {
  foreach i $args {
    $ved dataout status $i : got_do_stat $ved $i ? got_do_stat_err $ved $i
  }
}

proc got_do_list_err {ved conf} {
  #puts "got_do_list_err: ved $ved args $conf"
  #puts [$conf print]
  $conf delete
}

proc got_do_stat {ved do args} {
#  puts "got_do_stat ved $ved do $do args $args"
  set status [lindex $args 0]
}

proc got_do_stat_err {ved do conf} {
  #puts "got_do_stat_err ved $ved do $do args $conf"
  #puts [$conf print]
  $conf delete
}
