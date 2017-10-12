# $ZEL: ved_setup.tcl,v 1.1 2000/07/15 21:37:03 wuestner Exp $
#
# global vars in this file:
# list   global_veds
#
# string  static_ved(name)
# string  static_ved(proc)
# boolean static_ved(win_pos)           / true: Window ist bereits positioniert
#

proc dummy_VED {} {}

namespace eval VED {

# string  : name of selected VED
variable name
# boolean : true: window already positioned
variable win_pos
# window
variable listboxwidget

proc apply {} {
  variable name

  if [ems_connected] {
    open_ved $name
  } else {
    bell
  }
}

proc ok {window} {
  apply
  wm withdraw $window
}

proc cancel {window} {
  wm withdraw $window
}

proc fill_list {} {
  variable listboxwidget

  $listboxwidget delete 0 end
  if {![ems_connected]} {
    connect_commu
  }
  if [ems_connected] {
    if [catch {set veds [ems_veds]} mist] {
      LOG::put "read vedlist: $mist" tag_red
    } else {
      foreach i [lsort $veds] {
        $listboxwidget insert end $i
      }
      if {[$listboxwidget size]<10} {
        $listboxwidget config -height 0
      } else {
        $listboxwidget config -height 10
      }
    }
  }
}

proc win_open {} {
  variable win_pos

  if {![winfo exists .ved]} {win_create}
  if $win_pos then {wm positionfrom .ved user}
  fill_list
  wm deiconify .ved
  set win_pos 1
}

proc win_create {} {
  variable listboxwidget
  variable win_pos
  variable name

  bind Entry <Delete> {
         tkEntryBackspace %W
  }
  set name {}
  set win_pos 0
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
  entry $self.nameframe.e -textvariable VED::name -width 20
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
    set VED::name [selection get]
  }
  bind $self.listframe.b <KeyPress-Return> "
    set VED::name \[selection get\]
    $self.b_frame.a flash
    apply
  "
  bind $self.listframe.b <KeyPress-KP_Enter> {
    set VED::name [selection get]
  }
  pack $self.listframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.listframe.b -side left -fill both -expand 1
  pack $self.listframe.s -side right -fill y
  set listboxwidget $self.listframe.b

# buttonframe
  button $self.b_frame.ok -text OK -command "VED::ok $self"
  button $self.b_frame.a -text Open -default active -command VED::apply
  button $self.b_frame.c -text Cancel -command "VED::cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.c\
      -side left -fill both -padx 1m -pady 1m -expand 1

  pack $self.nameframe -side top -fill x -expand 0
  pack $self.listframe -side top -fill both -expand 1
  pack $self.b_frame -side bottom -fill x -expand 0

  bind $self.nameframe.e <Key-Return> "$self.b_frame.a flash; VED::apply"
}

}
