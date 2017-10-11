# $ZEL: scaler_print_win.tcl,v 1.3 2011/11/27 00:13:42 wuestner Exp $
# copyright 1998-2011
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

proc print_reset {} {
  global global_setup static_print

  set static_print(auto)    $global_setup(print_auto)
  set static_print(command) $global_setup(print_command)
}

proc print_apply {} {
  global global_setup static_print

  set global_setup(print_auto)    $static_print(auto)
  set global_setup(print_command) $static_print(command)
}

proc print_ok {window} {
  print_apply
  wm withdraw $window
}

proc print_cancel {window} {
  print_reset
  wm withdraw $window
}

proc print_win_open {} {
  global win_pos static_print

  if {![winfo exists .scaler_print]} {print_win_create}
  if $win_pos(print) then {wm positionfrom $static_print(win) user}
  print_reset
  wm deiconify $static_print(win)
  set win_pos(print) 1
}

proc print_win_create {} {
  global win_pos static_print

  set win_pos(print) 0
  set self .scaler_print
  set static_print(win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] print"
  wm maxsize $self 10000 10000

  frame $self.autoframe -relief raised -borderwidth 1
  frame $self.commframe -relief raised -borderwidth 1

  checkbutton $self.autoframe.c -text "automatic print"\
      -variable static_print(auto)
  pack $self.autoframe.c -side left

  label $self.commframe.l -text "Command :" -anchor w
  entry $self.commframe.e -textvariable static_print(command) -width 20
  pack $self.commframe.l -side top -fill x -expand 1
  pack $self.commframe.e -side top -fill x -expand 1

  frame $self.b_frame -relief raised -borderwidth 1

  button $self.b_frame.ok -text OK -command "print_ok $self"
  button $self.b_frame.a -text Apply -command "print_apply"
  button $self.b_frame.r -text Cancel -command "print_cancel $self"
  button $self.b_frame.n -text "Print now" -command print_scaler
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.n\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.b_frame -side bottom -fill both -expand 1
  pack $self.autoframe $self.commframe -side top -fill x -anchor w
}
