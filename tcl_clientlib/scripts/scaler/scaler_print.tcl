# $ZEL: scaler_print.tcl,v 1.3 2011/11/27 00:13:17 wuestner Exp $
# copyright 1998-2011
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

proc print_scaler {} {
  global global_setup global_setup_names scaler_cont
  global global_num_channels global_setup_print

  set num 0
  for {set i 0} {$i<$global_num_channels} {incr i} {
    if {$global_setup_print($i)} {incr num}
  }

  if {$num==0} {
    output "no channels selected for printing"
    return;
  }

  set line "exec $global_setup(print_command) $num"
  for {set i 0} {$i<$global_num_channels} {incr i} {
    if {$global_setup_print($i)} {
      append line " $i \{$global_setup_names($i)\} $scaler_cont($i)"
    }
  }
  append line " >&/dev/console </dev/null &"
  eval $line
}

proc auto_print_scaler {} {
  global global_setup

  if {$global_setup(print_auto)} print_scaler
}
