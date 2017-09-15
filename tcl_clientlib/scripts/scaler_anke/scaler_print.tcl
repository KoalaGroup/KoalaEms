proc print_scaler {} {
  global global_setup global_setup_names scaler_cont scaler_words

  if {[info exists global_setup(num_print_channels)] == 0} {
    set global_setup(num_print_channels) 24
  }
  for {set i 0} {$i<$global_setup(num_print_channels)} {incr i} {
    set scaler_cont($i) [lindex [split $scaler_words($i) .] 0]
  }
  set line "exec $global_setup(print_command) $global_setup(num_print_channels)"
  for {set i 0} {$i<$global_setup(num_print_channels)} {incr i} {
    append line " $i \{$global_setup_names($i)\} $scaler_cont($i)"
  }
  append line " >&/dev/console </dev/null &"
  #puts $line
  eval $line
}

proc auto_print_scaler {} {
  global global_setup

  if {$global_setup(print_auto)} print_scaler
}
