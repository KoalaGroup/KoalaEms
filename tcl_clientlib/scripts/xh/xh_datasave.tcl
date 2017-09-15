proc datasave_fill_list {} {
  global static_datasave hi
  set win $static_datasave(listboxwidget)
  $win delete 0 end
  set list [$hi(win) arrays]
  set static_datasave(listsize) [llength $list]
  foreach i $list {
    $win insert end $i
  }
  if {[$win size]<10} {
    $win config -height 0
  } else {
    $win config -height 10
  }
}

proc datasave_reset {window} {
  global static_datasave
  set static_datasave(name) ""
  set static_datasave(fname) ""
  datasave_fill_list
}

proc datasave_apply {} {
  global static_datasave hi
  if {$static_datasave(name)==""} return
  set arr $static_datasave(name)
  set file [open $static_datasave(fname)_info "CREAT WRONLY"]
  puts $file $arr
  puts $file [$arr name]
  puts $file [$arr limit]
  puts $file [$hi(win) arrcget $arr -color]
  close $file
  $arr dump $static_datasave(fname)
}

proc datasave_ok {window} {
  datasave_apply
  wm withdraw $window
}

proc datasave_cancel {window} {
  wm withdraw $window
}

proc datasave_win_open {} {
  global win_pos

  if {![winfo exists .xh_datasave]} {create_datasave_win}
  if $win_pos(datasave) then {wm positionfrom .xh_datasave user}
  datasave_fill_list
  datasave_reset .xh_datasave
  wm deiconify .xh_datasave
  set win_pos(datasave) 1
  }

proc datasave_select {} {
  global global_setup static_datasave
  if {$static_datasave(listsize)} {set static_datasave(name) [selection get]}
  if {$static_datasave(name)==""} return
  set static_datasave(fname)\
      $global_setup(data_autodir)/saved_$static_datasave(name)
}

proc create_datasave_win {} {
  global global_setup win_pos static_datasave

  set win_pos(datasave) 0
  set self .xh_datasave
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] save data"
  wm maxsize $self 10000 10000

  frame $self.nameframe -relief raised -borderwidth 1
  frame $self.fnameframe -relief raised -borderwidth 1
  frame $self.listframe -relief raised -borderwidth 1

# nameframe
  label $self.nameframe.l -text "Name:" -anchor w
  entry $self.nameframe.e -textvariable static_datasave(name) -width 20
  bind $self.nameframe.e <Key-Return> "$self.b_frame.a flash; datasave_apply"
  bind $self.nameframe.e <Key-KP_Enter> "$self.b_frame.a flash; datasave_apply"
  pack $self.nameframe.l -side top -padx 1m -fill x -expand 1
  pack $self.nameframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# fnameframe
  label $self.fnameframe.l -text "Fileame:" -anchor w
  entry $self.fnameframe.e -textvariable static_datasave(fname) -width 20
  bind $self.fnameframe.e <Key-Return> "$self.b_frame.a flash; datasave_apply"
  bind $self.fnameframe.e <Key-KP_Enter> "$self.b_frame.a flash; datasave_apply"
  pack $self.fnameframe.l -side top -padx 1m -fill x -expand 1
  pack $self.fnameframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# listframe
  label $self.listframe.l -text "Available Arrays:" -anchor w -borderwidth 1
  listbox $self.listframe.b -relief raised -borderwidth 1\
      -width 0 -height 0\
      -yscrollcommand "$self.listframe.s set"\
      -selectmode browse
  scrollbar $self.listframe.s -command "$self.listframe.b yview"
  bind $self.listframe.b <ButtonRelease-1> "datasave_select"
  bind $self.listframe.b <KeyPress-Return> "datasave_select"
  bind $self.listframe.b <KeyPress-KP_Enter> "datasave_select"
  pack $self.listframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.listframe.b -side left -fill both -expand 1
  pack $self.listframe.s -side right -fill y
  set static_datasave(listboxwidget) $self.listframe.b

# buttonframe
  frame $self.b_frame -relief raised -borderwidth 1
  button $self.b_frame.ok -text OK -command "datasave_ok $self"
  button $self.b_frame.a -text Apply -command "datasave_apply"
  button $self.b_frame.r -text Reset -command "datasave_reset $self"
  button $self.b_frame.c -text Cancel -command "datasave_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.nameframe $self.fnameframe\
      $self.listframe -side top -fill both
  pack $self.b_frame -side bottom -fill both -expand 1
}
