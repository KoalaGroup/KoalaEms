# $ZEL: scaler_send_to_net.tcl,v 1.3 2006/08/13 17:32:08 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
# 

proc send_names_to_net {} {
  global global_running
  global global_xh_path
  global global_num_channels
  global global_setup_names

  if {!$global_running(graphical_display)} return

  for {set i 0} {$i<$global_num_channels} {incr i} {
    if [info exists global_setup_names($i)] {
      lappend message "name scaler_$i \{$global_setup_names($i)\}"
    } else {
      lappend message "name scaler_$i \{scaler_$i\}"
    }
  }
  if [catch {
        puts $global_xh_path $message
        flush $global_xh_path
      } mist] {
    output $mist
    set global_running(graphical_display) 0
  }
}

proc send_to_net {offs num jetzt} {
  global global_running
  global global_xh_path
  global scaler_time
  global scaler_time_last
  global scaler_cont
  global scaler_cont_last
  global global_setup_sub

  if {!$global_running(graphical_display)} return

  set xnum 0
  for {set i 0} {$i<$num} {incr i} {
    set idx [expr $i+$offs]
    if {($scaler_time_last($idx)>0) && ($scaler_time($idx)>$scaler_time_last($idx))} {
      set rate [expr ($scaler_cont($idx)-$scaler_cont_last($idx))/($scaler_time($idx)-$scaler_time_last($idx))]
      if [info exists global_setup_sub($idx)] {
        set rate [expr $rate-$global_setup_sub($idx)]
        if {$rate<0} {set rate 0}
      }
      if {$rate>=0} {
        lappend message "value scaler_$i $jetzt $rate"
        incr xnum
      }
    } 
  }

  if {$xnum==0} return
  if [catch {
        puts $global_xh_path $message
        flush $global_xh_path
      } mist] {
    output $mist
    set global_running(graphical_display) 0
  }
}
