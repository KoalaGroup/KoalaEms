# $ZEL: xh_axis.tcl,v 1.4 2004/11/18 12:07:44 wuestner Exp $
# copyright 1998 ... 2004
#   Peter Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

proc axis_apply {} {
  global global_setup hi x_axis_diff y_axis_max
  if {[llength $x_axis_diff]>1} {
    set l1 [lindex $x_axis_diff 0]
    set l2 [lindex $x_axis_diff 1]
    switch -regexp $l2 {
      [mM] {set l1 [expr $l1*60]}
      [sS] {}
      [hH] {set l1 [expr $l1*3600]}
      default {puts "$l2 ist Unsinn"; return}
    }
    set global_setup(x_axis_diff) $l1
  } else {
    set global_setup(x_axis_diff) $x_axis_diff
  }
  set global_setup(y_axis_max) $y_axis_max
  $hi(win) xdiff $global_setup(x_axis_diff)
  $hi(win) ymax $global_setup(y_axis_max)
  make_y_scale
}

proc axis_ok {window} {
  axis_apply
  wm withdraw $window
}

proc axis_reset {} {
  global global_setup
  global x_axis_diff y_axis_max
  set x_axis_diff $global_setup(x_axis_diff)
  set y_axis_max $global_setup(y_axis_max)
}

proc axis_cancel {window} {
  axis_reset
  wm withdraw $window
}

proc axis_win_open {} {
  global win_pos

  if {![winfo exists .axis]} {create_axis_win}
  if $win_pos(axis) then {wm positionfrom .axis user}
  wm deiconify .axis
  raise .axis
  set win_pos(axis) 1
}

proc create_axis_win {} {
  global global_setup win_pos
  global y_axis_max x_axis_diff

  set win_pos(axis) 0
  set y_axis_max $global_setup(y_axis_max)
  set x_axis_diff $global_setup(x_axis_diff)
  set self .axis
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] axes"
  wm maxsize $self 10000 10000

  frame $self.scal_frame -relief raised -borderwidth 1
  frame $self.scal_frame.x
  frame $self.scal_frame.y
  label $self.scal_frame.x.l -text "X range: "
  entry $self.scal_frame.x.e -textvariable x_axis_diff -width 20
  bind $self.scal_frame.x.e <Key-Return> "$self.b_frame.a flash;axis_apply"
  bind $self.scal_frame.x.e <Key-KP_Enter> "$self.b_frame.a flash;axis_apply"
  pack $self.scal_frame.x.l -side left -padx 2m -pady 2m
  pack $self.scal_frame.x.e -side right -fill x -padx 2m -pady 2m -expand 1
  label $self.scal_frame.y.l -text "Y range: "
  entry $self.scal_frame.y.e -textvariable y_axis_max -width 20
  bind $self.scal_frame.y.e <Key-Return> "$self.b_frame.a flash;axis_apply"
  bind $self.scal_frame.y.e <Key-KP_Enter> "$self.b_frame.a flash;axis_apply"
  pack $self.scal_frame.y.l -side left -padx 2m -pady 2m
  pack $self.scal_frame.y.e -side right -fill x -padx 2m -pady 2m -expand 1
  pack $self.scal_frame.x $self.scal_frame.y -fill x

  frame $self.b_frame -relief raised -borderwidth 1
  button $self.b_frame.ok -text OK -command "axis_ok $self"
  button $self.b_frame.a -text Apply -command "axis_apply"
  button $self.b_frame.r -text Reset -command "axis_reset"
  button $self.b_frame.c -text Cancel -command "axis_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
                  -side left -fill both -padx 2m -pady 2m -expand 1
  pack $self.scal_frame -side top -fill both
  pack $self.b_frame -side bottom -fill both -expand 1
}
