# $ZEL: scaler_menu_commands.tcl,v 1.6 2014/08/19 20:20:50 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# start_stop
# global_dispwin
# global_running(graphical_display)
# global_running(numerical_display)
# global_setup(graphical_display)
# global_setup(numerical_display)
# global_running(outer_loop)
# global_after(outer_loop)
#

#-----------------------------------------------------------------------------#
proc start_stop {} {
  global global_running

  if {$global_running(outer_loop)} {
    stop_outer_loop
  } else {
    start_outer_loop
  }
}
#-----------------------------------------------------------------------------#
proc update_start_stop {args} {
  global global_running start_stop

  if {$global_running(outer_loop)} {
    set start_stop Stop
  } else {
    set start_stop Start
  }
}
#-----------------------------------------------------------------------------#
proc start_outer_loop {} {
  global global_setup
  global global_running
  global start_stop

  set global_running(graphical_display) 0
  set global_running(numerical_display) 0

  if {$global_setup(graphical_display)} {
    if {[connect_xh]!=0} {
        return -1
    }
    set global_running(graphical_display) 1
  }
  send_names_to_net
  if {$global_setup(numerical_display)} {
    create_display
    set global_running(numerical_display) 1
    repack_all
  }
  set global_running(outer_loop) 1
  update idletasks
  set global_after(outer_loop) [after 1000 outer_loop]
}
#-----------------------------------------------------------------------------#
proc stop_outer_loop {} {
  global global_setup global_after
  global global_running

  output stop_outer_loop
  update idletasks
  set global_running(outer_loop) 0
  foreach i [array names global_after] {
    after cancel $global_after($i)
  }
  disconnect_commu stop_outer_loop
  if {$global_running(graphical_display)} {
    disconnect_xh
    set global_running(graphical_display) 0
  }
  if {$global_running(numerical_display)} {
    destroy_display
    set global_running(numerical_display) 0
    repack_all
  }
  .bar configure -background #f00
}
#-----------------------------------------------------------------------------#
proc create_menu_commands {parent} {
  global start_stop global_running

  set self [string trimright $parent .].commands
  update_start_stop
  button $self -textvariable start_stop -underline 0 -relief flat\
    -highlightthickness 0\
    -command start_stop
  trace variable global_running(outer_loop) w update_start_stop
  return $self
}
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
