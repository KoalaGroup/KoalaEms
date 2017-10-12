# $Id: update_status.tcl,v 1.9 2000/08/10 09:26:50 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

# callbacks:

proc got_ro_status {ved args} {
  global global_rostatvar_$ved
  global global_rostatvar_num_$ved
  global global_status_loop
  #output "got_ro_status: $ved $args" tag_orange
  set text $args
  set ev [lindex $args 1]
  if {[lindex $args 0]=="running"} {
    set old [set global_rostatvar_num_${ved}(ro)]
    append text " [format {(%.1f ev/s)} [expr ($ev-$old)*1000./$global_status_loop(delay)]]"
  }
  set global_rostatvar_${ved}(ro) $text
  set global_rostatvar_num_${ved}(ro) $ev  
}

proc got_ro_status_error {ved conf} {
  # output "got_ro_status_error: $ved $conf" tag_orange
  output "readout $ved: [$conf print text]" tag_orange
  $conf delete
}

proc got_do_status {ved idx args} {
  global global_rostatvar_$ved
  global global_rostatvar_num_$ved
  global global_status_loop
  global global_intsize
  #output "got_do_status: $ved $idx $args" tag_orange
  set key ${ved}_${idx}
  set stat [statnames [lindex $args 1]] ; # do status
  set doswitch [lindex $args 2] ; # do switch
  set bytes [lindex $args 5]
  set text $stat
  if {$doswitch==1} {append text " (disabled)"}
  set err [lindex $args 0] ; # do error
  if {$err} {append text " (error $err)"}
  #append text " [format {%d} [lindex $args 4]]"
  if {$stat=="running" || $stat=="flushing"} {
    set old [set global_rostatvar_num_${ved}($idx)]
    append text " [format {(%.3f MB/s)} [expr ($bytes-$old)*4000./$global_status_loop(delay)/1048576.0]]"
  }
  set global_rostatvar_${ved}($idx) $text
  set global_rostatvar_num_${ved}($idx) $bytes
  global global_dataout_$key
  if [set global_dataout_${key}(istape)] {
    if {$global_intsize<64} {
      update_tstatus_32 $key [lrange $args 11 end]
    } else {
      update_tstatus_64 $key [lrange $args 11 end]
    }
  }
}

proc got_do_status_error {ved do conf} {
  # output "got_do_status_error: $ved $do $conf" tag_orange
  output "$ved dataout $do: [$conf print text]" tag_orange
  $conf delete
}

proc status_update_do {ved idx} {
  #output "status_update_do $ved $idx"
  set confmode [$ved confmode asynchron]
  if [catch {
    $ved dataout status $idx 1 : got_do_status $ved $idx ? got_do_status_error $ved $idx
    } mist] {
    output "status_update_do $ved do $idx: $mist" tag_blue
  }
  $ved confmode $confmode
}

proc status_update_ved {ved} {
  global global_dataoutlist

  #output "status_update_ved $ved"
  set confmode [$ved confmode asynchron]
  if [catch {
    $ved readout status : got_ro_status $ved ? got_ro_status_error $ved
  } mist ] {
    output "status_update_ved $ved: $mist" tag_blue
  } else {
    foreach idx $global_dataoutlist($ved) {
      status_update_do $ved $idx
    }
  }
  $ved confmode $confmode
}

proc status_update {} {
  global global_veds
  #output "status_update" tag_blue
  foreach i [array names global_veds] {
    status_update_ved $global_veds($i)
  }
}

proc status_loop {} {
  global global_status_loop
  #output "status_loop"
  status_update
  set global_status_loop(id) [after $global_status_loop(delay) status_loop]
}

proc start_status_loop {} {
  global global_status_loop

  if [info exists global_status_loop(id)] {
    after cancel $global_status_loop(id)
  }
  set global_status_loop(delay) 5000
  #set global_status_loop(id) [after $global_status_loop(delay) status_loop]
  set global_status_loop(id) [after 100 status_loop]
  #output "start_status_loop"
}

proc stop_status_loop {} {
  global global_status_loop

  if [info exists global_status_loop(id)] {
    after cancel $global_status_loop(id)
    unset global_status_loop(id)
  }
}
