proc datarestore_fill_list {} {
  global static_datarestore
  set win $static_datarestore(listboxwidget)
  $win delete 0 end
  set list [glob $static_datarestore(filter)]
  set static_datarestore(listsize) [llength $list]
  foreach i $list {
    if {[regexp {_info$} $i]==0} {$win insert end $i}
  }
  if {[$win size]<10} {
    $win config -height 0
  } else {
    $win config -height 10
  }
}

proc datarestore_reset {} {
  global global_setup static_datarestore
  set static_datarestore(name) ""
  set static_datarestore(fname) ""
  set static_datarestore(filter) $global_setup(data_autodir)/*
  datarestore_fill_list
}

proc datarestore_filter {} {
  global static_datarestore
  set static_datarestore(name) ""
  set static_datarestore(fname) ""
  datarestore_fill_list
}

proc datarestore_apply {} {
  global static_datarestore hi global_arrays
  if {$static_datarestore(fname)==""} return
  
  if [catch {set file [open $static_datarestore(fname)_info "RDONLY"]} mist] {
    bgerror $mist
  } else {
    
    if {$static_datarestore(name)==""} {
      set arr [gets $file]
    } else {
      set arr $static_datarestore(name)
      set dummyarr [gets $file]
    }
    set name [gets $file]
    set limit [gets $file]
    set color [gets $file]
    if {[info exists global_arrays($arr)]==0} {
      histoarray $arr $name
    }
    $arr limit $limit
    close $file
    $arr restore $static_datarestore(fname)
    if {[info exists global_arrays($arr)]==0} {
      $hi(win) attach $arr -color $color
      set global_arrays($arr) ""
    }
  }
}

proc datarestore_ok {window} {
  datarestore_apply
  wm withdraw $window
}

proc datarestore_cancel {window} {
  wm withdraw $window
}

proc datarestore_win_open {} {
  global win_pos

  if {![winfo exists .xh_datarestore]} {create_datarestore_win}
  if $win_pos(datarestore) then {wm positionfrom .xh_datarestore user}
  datarestore_fill_list
  datarestore_reset
  wm deiconify .xh_datarestore
  set win_pos(datarestore) 1
  }

proc datarestore_select {} {
  global global_setup static_datarestore
  if {$static_datarestore(listsize)} {
    set static_datarestore(fname) [selection get]
  }
  if {$static_datarestore(fname)==""} return
  if [catch {set file [open $static_datarestore(fname)_info "RDONLY"]} mist] {
  } else {
    set static_datarestore(name) [gets $file]
    close $file
  }
}

proc create_datarestore_win {} {
  global global_setup win_pos static_datarestore

  set win_pos(datarestore) 0
  set self .xh_datarestore
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] restore data"
  wm maxsize $self 10000 10000

  frame $self.nameframe -relief raised -borderwidth 1
  frame $self.fnameframe -relief raised -borderwidth 1
  frame $self.filterframe -relief raised -borderwidth 1
  frame $self.listframe -relief raised -borderwidth 1

# nameframe
  label $self.nameframe.l -text "Name:" -anchor w
  entry $self.nameframe.e -textvariable static_datarestore(name) -width 20
  bind $self.nameframe.e <Key-Return> "$self.b_frame.a flash; datarestore_apply"
  bind $self.nameframe.e <Key-KP_Enter> "$self.b_frame.a flash; datarestore_apply"
  pack $self.nameframe.l -side top -padx 1m -fill x -expand 1
  pack $self.nameframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# fnameframe
  label $self.fnameframe.l -text "Filename:" -anchor w
  entry $self.fnameframe.e -textvariable static_datarestore(fname) -width 20
  bind $self.fnameframe.e <Key-Return> "$self.b_frame.a flash; datarestore_apply"
  bind $self.fnameframe.e <Key-KP_Enter> "$self.b_frame.a flash; datarestore_apply"
  pack $self.fnameframe.l -side top -padx 1m -fill x -expand 1
  pack $self.fnameframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# filterframe
  label $self.filterframe.l -text "Filter:" -anchor w
  entry $self.filterframe.e -textvariable static_datarestore(filter) -width 20
  bind $self.filterframe.e <Key-Return> "$self.b_frame.a flash; datarestore_filter"
  bind $self.filterframe.e <Key-KP_Enter> "$self.b_frame.a flash; datarestore_filter"
  pack $self.filterframe.l -side top -padx 1m -fill x -expand 1
  pack $self.filterframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# listframe
  label $self.listframe.l -text "Available Files:" -anchor w -borderwidth 1
  listbox $self.listframe.b -relief raised -borderwidth 1\
      -width 0 -height 0\
      -yscrollcommand "$self.listframe.s set"\
      -selectmode browse
  scrollbar $self.listframe.s -command "$self.listframe.b yview"
  bind $self.listframe.b <ButtonRelease-1> "datarestore_select"
  bind $self.listframe.b <KeyPress-Return> "datarestore_select"
  bind $self.listframe.b <KeyPress-KP_Enter> "datarestore_select"
  pack $self.listframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.listframe.b -side left -fill both -expand 1
  pack $self.listframe.s -side right -fill y
  set static_datarestore(listboxwidget) $self.listframe.b

# buttonframe
  frame $self.b_frame -relief raised -borderwidth 1
  button $self.b_frame.ok -text OK -command "datarestore_ok $self"
  button $self.b_frame.a -text Apply -command "datarestore_apply"
  button $self.b_frame.r -text Reset -command "datarestore_reset"
  button $self.b_frame.c -text Cancel -command "datarestore_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $self.nameframe $self.fnameframe $self.filterframe\
      $self.listframe -side top -fill both
  pack $self.b_frame -side bottom -fill both -expand 1
}
