proc got_ro_stat {ved args} {
  #puts "got_ro_stat ved $ved args $args"
  set name [$ved name]
  set status [lindex $args 0]
  switch $status {
    error        {.disp.$name.name configure -background #f00}
    no_more_data {.disp.$name.name configure -background #0f0}
    stopped      {.disp.$name.name configure -background #ff0}
    inactive     {.disp.$name.name configure -background #bbb}
    running      {.disp.$name.name configure -background #d9d9d9}
  }
}

proc got_ro_stat_err {ved conf} {
  #puts "got_ro_stat_err ved $ved do $do args $conf"
  #puts [$conf print]
  $conf delete
}
