#
# Diese Prozedur erzeugt das Actionsmenu im Hauptmenu
#

proc create_menu_actions {parent} {
  global global_actions_menu
  
  set self [string trimright $parent .].actions
  menubutton $self -text "Actions" -menu $self.m -underline 0
  set global_actions_menu(win) \
      [menu $self.m -tearoff 1 -tearoffcommand {offtorn global_actions_menu}]
  lappend global_actions_menu(tornoffs) $global_actions_menu(win)
  set entry 0

  $self.m add command -label "Initialize" -underline 0\
      -command scaler_init
  set global_actions_menu(init_entry) $entry
  incr entry

  $self.m add command -label "Start" -underline 0\
      -command scaler_run
  set global_actions_menu(start_entry) $entry
  incr entry

  $self.m add command -label "Stop" -underline 0\
      -command scaler_stop
  set global_actions_menu(stop_entry) $entry
  incr entry

  $self.m add command -label "Reset" -underline 0\
      -command scaler_reset
  set global_actions_menu(reset_entry) $entry
  incr entry

  return $self
}

proc enable_actions {val} {
  global global_actions_menu
  checktornoffs  global_actions_menu
  foreach win $global_actions_menu(tornoffs) {
    if {$win==$global_actions_menu(win)} {
      set offs 1
    } else {
      set offs 0
    }
    switch -exact $val {
      uninitialized {
        $win entryconfigure \
          [expr $global_actions_menu(init_entry)+$offs] -state normal
        $win entryconfigure \
          [expr $global_actions_menu(start_entry)+$offs] -state normal
        $win entryconfigure \
          [expr $global_actions_menu(stop_entry)+$offs] -state disabled
        $win entryconfigure \
          [expr $global_actions_menu(reset_entry)+$offs] -state disabled
        enable_commu_setup 1
      }
      initialized {
        $win entryconfigure \
          [expr $global_actions_menu(init_entry)+$offs] -state disabled
        $win entryconfigure \
          [expr $global_actions_menu(start_entry)+$offs] -state normal
        $win entryconfigure \
          [expr $global_actions_menu(stop_entry)+$offs] -state disabled
        $win entryconfigure \
          [expr $global_actions_menu(reset_entry)+$offs] -state normal
        enable_commu_setup 0

      }
      running {
        $win entryconfigure \
          [expr $global_actions_menu(init_entry)+$offs] -state disabled
        $win entryconfigure \
          [expr $global_actions_menu(start_entry)+$offs] -state disabled
        $win entryconfigure \
          [expr $global_actions_menu(stop_entry)+$offs] -state normal
        $win entryconfigure \
          [expr $global_actions_menu(reset_entry)+$offs] -state disabled
        enable_commu_setup 0
      }
    }
  }
}
