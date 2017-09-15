proc main_loop_ {} {
  global global_setup global_main_loop 
  global global_commuerror global_commuerror_x global_vederror

  if {![ems_connected]} {
    if {[connect_commu 0]!=0} {
      if {![info exists global_commuerror]
          || ($global_commuerror!=$global_commuerror_x)} {
        set global_commuerror $global_commuerror_x
        timestamp 1
        output_append "connect to commu: $global_commuerror"
        output_append "will retry every $global_setup(retryinterval) seconds."
      }
    } else {
      timestamp 1
      output_append "commu connected"
      if {[info exists global_commuerror]} {unset global_commuerror}
      if {[info exists global_vederror]} {unset global_vederror}
    }
  }

  if [ems_connected] {
    set ov [ems_openveds]
    set sv $global_setup(veds)
    foreach v $ov {
      set n [$v name]
      set idx [lsearch $sv $n]
      if {$idx>=0} {
        set sv [lreplace $sv $idx $idx]
      }
    }
    foreach v $sv {
      if [catch {set vv [ems_open $v]} mist] {
        if {![info exists global_vederror($v)]
            || ($global_vederror($v)!=$mist)} {
          set global_vederror($v) $mist
          timestamp 1
          output_append "open ved $v: $global_vederror($v)"
          output_append "will retry every $global_setup(retryinterval) seconds."
        }
      } else {
        timestamp 1
        output_append "open ved $v OK."
        if {[info exists global_vederror($v)]} {unset global_vederror($v)}
        # $vv unsol {print_ved_unsol %v %h %d}
      }
    }
  }

  set global_main_loop(id) [
    after [expr 1000*$global_setup(retryinterval)] main_loop_
  ]
}

proc start_main_loop {} {
  global global_main_loop

  if {$global_main_loop(running)} {
    output "Warning in start_main_loop: main loop already runs."
    start_stop_menu_change
  } else {
    set global_main_loop(running) 1
    start_stop_menu_change
    main_loop_
  }
}

proc stop_main_loop {} {
  global global_main_loop
  if [info exists global_main_loop(id)] {
    after cancel $global_main_loop(id)
  }
  set global_main_loop(running) 0
  start_stop_menu_change
}

proc restart_main_loop {} {
  stop_main_loop
  start_main_loop
}
