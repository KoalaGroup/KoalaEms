#
# global vars in this file:
#
# window global_menu(win)
# boolean global_<vedname>(readout)
#

#proc insert_ved_menu_button {} {
#  set mbutts [winfo children $menu(win)]
#  set last [lindex $mbutts [expr {[llength $mbutts]-2}]]
#  puts "last button: $last"
#  pack
#}

proc ved_any_commands {win ved} {
  $win add command -label "Close" -underline 0 -command "close_ved $ved"
  $win add command -label "Identify" -underline 0 -command "ved_identify $ved"
}

proc ved_controller_commands {win ved} {
  global global_$ved
  set global_${ved}(readout) 1

  $win add command -label "Variables..." -command "var_win_open $ved"\
      -underline 0
  $win add command -label "List of Procedures" -underline 8\
      -command "ved_caplist $ved proc"
  $win add command -label "List of Triggerprocedures" -underline 8\
      -command "ved_caplist $ved trig"
  $win add command -label "Trigger (im Bau)" -underline 1\
      -command "trig_win_open $ved"
  $win add checkbutton -label "ReadOut enabled" -underline 8\
      -variable global_${ved}(readout) -selectcolor black
  $win add command -label "Modullist" -underline 0\
      -command "modullist_win_open $ved"
  $win add command -label "Create IS (im Bau)" -underline 0\
      -command "is_win_open $ved"
  $win add command -label "Execute Command" -underline 1\
      -command "docommand_win_open $ved"
}

proc ved_camac_commands {win ved} {
}

proc ved_fastbus_commands {win ved} {
}

proc ved_emin_commands {win ved} {
}

proc ved_emout_commands {win ved} {
}

proc create_menu_ved_any {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}

proc create_menu_ved_eventmanager {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  ved_emin_commands $self.m $ved
  ved_emout_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}

proc create_menu_ved_controller {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  ved_controller_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}

proc create_menu_ved_chimaere {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  ved_controller_commands $self.m $ved
  ved_emout_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}

proc create_menu_ved_camac {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  ved_controller_commands $self.m $ved
  ved_camac_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}

proc create_menu_ved_fastbus {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  ved_controller_commands $self.m $ved
  ved_fastbus_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}

proc create_menu_ved_camac_chimaere {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  ved_controller_commands $self.m $ved
  ved_camac_commands $self.m $ved
  ved_emout_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}

proc create_menu_ved_fastbus_chimaere {ved} {
  global global_menu
  set name [string range $ved 4 end]
  set self [string trimright $global_menu(win) .].$name
  menubutton $self -text $name -menu $self.m
  menu $self.m -tearoff 1
  ved_any_commands $self.m $ved
  ved_controller_commands $self.m $ved
  ved_fastbus_commands $self.m $ved
  ved_emout_commands $self.m $ved
  pack $self -side left -padx 1m
  return $self
}
