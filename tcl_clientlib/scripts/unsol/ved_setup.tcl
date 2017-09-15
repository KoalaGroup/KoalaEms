#
# global vars in this file:
# list   global_veds
#
# string  static_ved(name)
# string  static_ved(proc)
# boolean static_ved(win_pos)           / true: Window ist bereits positioniert
#

proc ved_apply {} {
  global global_setup global_veds static_ved
  if [ems_connected] {
    open_ved $static_ved(name)
  } else {
    bell
  }
}

proc ved_ok {window} {
  ved_apply
  wm withdraw $window
}

proc ved_cancel {window} {
  wm withdraw $window
}

proc ved_fill_list {} {
  global static_ved
  $static_ved(listboxwidget) delete 0 end
  if {![ems_connected]} {
    connect_commu 1
  }
  if [ems_connected] {
    foreach i [lsort [ems_veds]] {
      $static_ved(listboxwidget) insert end $i
    }
    if {[$static_ved(listboxwidget) size]<10} {
      $static_ved(listboxwidget) config -height 0
    } else {
      $static_ved(listboxwidget) config -height 10
    }
  }
}

proc ved_win_open {} {
  global static_ved win_pos

  if {![winfo exists .ved]} {ved_win_create}
  if $win_pos(ved) then {wm positionfrom .ved user}
  ved_fill_list
  wm deiconify .ved
  set win_pos(ved) 1
}

proc ved_win_create {} {
  global global_setup static_ved win_pos

  bind Entry <Delete> {
         tkEntryBackspace %W
  }
  set static_ved(name) {}
  set win_pos(ved) 0
  set self .ved
  toplevel $self
  wm group $self .
  wm title $self "[winfo name .] open ved"
  wm maxsize $self 10000 10000

  frame $self.nameframe -relief raised -borderwidth 1
  frame $self.listframe -relief sunken -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# nameframe
  label $self.nameframe.l -text "VEDname:" -anchor w
  entry $self.nameframe.e -textvariable static_ved(name) -width 20
  pack $self.nameframe.l -side top -padx 1m -fill x -expand 1
  pack $self.nameframe.e -side top -padx 1m -pady 1m -fill x -expand 1

# listframe
  label $self.listframe.l -text "Available VEDs:" -anchor w -borderwidth 1
  listbox $self.listframe.b -relief raised -borderwidth 1\
      -width 0 -height 0\
      -yscrollcommand "$self.listframe.s set"\
      -selectmode browse
  scrollbar $self.listframe.s -command "$self.listframe.b yview"
  bind $self.listframe.b <ButtonRelease-1> {
    set static_ved(name) [selection get]
  }
  bind $self.listframe.b <KeyPress-Return> "
    set static_ved(name) \[selection get\]
    $self.b_frame.a flash
    ved_apply
  "
  bind $self.listframe.b <KeyPress-KP_Enter> {
    set static_ved(name) [selection get]
  }
  pack $self.listframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.listframe.b -side left -fill both -expand 1
  pack $self.listframe.s -side right -fill y
  set static_ved(listboxwidget) $self.listframe.b

# buttonframe
  button $self.b_frame.ok -text OK -command "ved_ok $self"
  button $self.b_frame.a -text Open -default active -command "ved_apply"
  button $self.b_frame.c -text Cancel -command "ved_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.c\
      -side left -fill both -padx 1m -pady 1m -expand 1

  pack $self.nameframe -side top -fill x -expand 0
  pack $self.listframe -side top -fill both -expand 1
  pack $self.b_frame -side bottom -fill x -expand 0

  bind $self.nameframe.e <Key-Return> "$self.b_frame.a flash; ved_apply"
}
