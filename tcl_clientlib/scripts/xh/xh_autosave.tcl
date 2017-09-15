proc autosave_reset {} {
  global global_setup static_autosave

  set static_autosave(autosave) $global_setup(data_autosave)
  set static_autosave(autorestore) $global_setup(data_autorestore)
  set static_autosave(autodir) $global_setup(data_autodir)
}

proc autosave_apply {} {
  global global_setup static_autosave
  set global_setup(data_autosave) $static_autosave(autosave)
  set global_setup(data_autorestore) $static_autosave(autorestore)
  set global_setup(data_autodir) $static_autosave(autodir)
}

proc autosave_ok {window} {
  autosave_apply
  wm withdraw $window
}

proc autosave_cancel {window} {
  autosave_reset
  wm withdraw $window
}

proc autosave_win_open {} {
  global win_pos

  if {![winfo exists .autosave]} {create_autosave_win}
  if $win_pos(autosave) then {wm positionfrom .autosave user}
  autosave_reset
  wm deiconify .autosave
  set win_pos(autosave) 1
}

proc create_autosave_win {} {
  global global_setup win_pos  static_autosave
  
  set win_pos(autosave) 0
  set self .autosave
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] autosave"
  wm maxsize $self 10000 10000

  frame $self.saveframe -relief raised -borderwidth 1
  frame $self.restoreframe -relief raised -borderwidth 1
  frame $self.dirframe -relief raised -borderwidth 1
  
# saveframe
  checkbutton $self.saveframe.c\
      -text "Autosave Data"\
      -variable static_autosave(autosave)\
      -selectcolor black\
      -justify left
  pack $self.saveframe.c -side left -padx 1m -pady 1m -fill x

# restoreframe
  checkbutton $self.restoreframe.c\
      -text "Autorestore Data"\
      -variable static_autosave(autorestore)\
      -selectcolor black
  pack $self.restoreframe.c -side left -padx 1m -pady 1m -fill x

# dirframe
  label $self.dirframe.l -text "Directory:" -anchor w
  entry $self.dirframe.e -textvariable static_autosave(autodir) -width 20
  pack $self.dirframe.l -side top -padx 1m -fill x -expand 1
  pack $self.dirframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# buttonframe
  frame $self.b_frame -relief raised -borderwidth 1
  button $self.b_frame.ok -text OK -command "autosave_ok $self"
  button $self.b_frame.a -text Apply -command "autosave_apply"
  button $self.b_frame.r -text Reset -command "autosave_reset"
  button $self.b_frame.c -text Cancel -command "autosave_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.saveframe $self.restoreframe $self.dirframe\
      -side top -fill both
  pack $self.b_frame -side bottom -fill both -expand 1
}
