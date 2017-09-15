#
# start_stop
# global_dispwin
# global_running(graphical_display)
# global_running(numerical_display)
# global_setup(graphical_display)
# global_setup(numerical_display)
# global_setup(use_gui)
# global_loop_running
# global_after(id)
#

#-----------------------------------------------------------------------------#

proc start_stop {} {
  global start_stop
  if {$start_stop=="Start"} {
    if {[loop_start]==0} {set start_stop Stop}
  } else {
    loop_stop
    set start_stop Start
  }
}

#-----------------------------------------------------------------------------#

proc loop_start {} {
  global global_setup global_loop_running
  global global_running

  set global_running(graphical_display) 0
  set global_running(numerical_display) 0

  if {$global_setup(graphical_display)} {
    if {[connect_xh]!=0} {return -1}
    set global_running(graphical_display) 1
  }
  if {$global_setup(numerical_display)} {
    create_display
    set global_running(numerical_display) 1
    repack_all
  }
  set global_loop_running 1
  if {[connect_loop]!=0} {return -1}
  if {$global_setup(use_gui)} {.bar configure -background #00f}
  return 0
}

#-----------------------------------------------------------------------------#

proc loop_stop {} {
  global global_setup global_loop_running global_after
  global global_running

  set global_loop_running 0
  if [info exists global_after(id)] {after cancel $global_after(id)}
  disconnect_commu
  if {$global_running(graphical_display)} {
    disconnect_xh
    set global_running(graphical_display) 0
  }
  if {$global_running(numerical_display)} {
    destroy_display
    set global_running(numerical_display) 0
    repack_all
  }
  if {$global_setup(use_gui)} {.bar configure -background #f00}
}

#-----------------------------------------------------------------------------#

proc create_menu_commands {parent} {
  global start_stop

  set self [string trimright $parent .].commands
  set start_stop Start
  button $self -textvariable start_stop -underline 0 -relief flat\
    -highlightthickness 0\
    -command start_stop
}

#-----------------------------------------------------------------------------#

proc create_menu_print {parent} {
  set self [string trimright $parent .].print
  button $self -text Print -underline 0 -relief flat\
    -highlightthickness 0\
    -command print_scaler
}

#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
