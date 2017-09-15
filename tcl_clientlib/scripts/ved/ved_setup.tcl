#
# global vars in this file:
#
# boolean global_setup(offline)
# string  global_setup(vedtype_font)
#
# string  static_ved(name)
# string  static_ved(proc)
# string  static_ved(listboxwidget)
# window  static_ved(selectedtype)
# window  static_ved(selectedboxe)
# window  static_ved(selectedboxt)
# boolean static_ved(win_pos)           / true: Window ist bereits positioniert
#

#proc ved_apply {window} {
#  global global_setup static_ved
#  if {$global_setup(offline) == 0} {
#    if "[ems_connected]==0" {
#      connect_commu
#    }
#    if [ems_connected] {
#      open_ved $static_ved(name) [lindex {any eventmanager controller chimaere\
#          camac fastbus camac_chimaere fastbus_chimaere}\
#          $static_ved(selectedtype)]
#    }
#  } else {
#    open_ved_fake $static_ved(name) [lindex {any eventmanager controller\
#        chimaere camac fastbus camac_chimaere fastbus_chimaere}\
#        $static_ved(selectedtype)]
#  }
#}

proc ved_apply {} {
  global global_setup static_ved
  if {$global_setup(offline) == 0} {
    if "[ems_connected]==0" {
      connect_commu
    }
    if [ems_connected] {
      open_ved $static_ved(name) $static_ved(selectedtype)
    }
  } else {
    open_ved_fake $static_ved(name) $static_ved(selectedtype)
  }
}

proc ved_ok {window} {
  ved_apply
  wm withdraw $window
}

proc ved_cancel {window} {
  ved_reset
  wm withdraw $window
}

proc ved_fill_list {} {
  global static_ved
  $static_ved(listboxwidget) delete 0 end
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

proc ved_reset {} {
  global static_ved

  set win $static_ved(cv)
  set static_ved(name) {}
  ved_fill_list
  foreach i [$win find withtag LAST] {
    if {[lsearch -regexp [$win gettags $i] TEXT]>=0} {
      $win itemconfig $i -fill black
    } else {
      $win itemconfig $i -fill [$win cget -background]
    }
  }
  $win dtag LAST
  set tags [$win gettags [lindex [$win find withtag DEF] 0]]
  set tag [lindex $tags [lsearch -regexp $tags type_]]
  $win addtag LAST withtag DEF
  foreach i [$win find withtag LAST] {
    if {[lsearch -regexp [$win gettags $i] TEXT]>=0} {
      $win itemconfig $i -fill white
    } else {
      $win itemconfig $i -fill black
    }
  }
  regsub type_ $tag {} type
  set static_ved(selectedtype) $type
}

proc any_enter {win} {
  set id [$win find withtag current]
  if {[lsearch -exact [$win gettags $id] TEXT]>=0} {
    set id [$win find below $id]
  }
  $win itemconfig $id -width 2
}

proc any_leave {win} {
  set id [$win find withtag current]
  if {[lsearch -exact [$win gettags $id] TEXT]>=0} {
    set id [$win find below $id]
  }
  $win itemconfig $id -width 1
}

proc entered {win} {
  global static_ved

  foreach i [$win find withtag LAST] {
    if {[lsearch -regexp [$win gettags $i] TEXT]>=0} {
      $win itemconfig $i -fill black
    } else {
      $win itemconfig $i -fill [$win cget -background]
    }
  }
  $win dtag LAST
  set tags [$win gettags [$win find withtag current]]
  set tag [lindex $tags [lsearch -regexp $tags type_]]
  $win addtag LAST withtag $tag
  foreach i [$win find withtag LAST] {
    if {[lsearch -regexp [$win gettags $i] TEXT]>=0} {
      $win itemconfig $i -fill white
    } else {
      $win itemconfig $i -fill black
    }
  }
  regsub type_ $tag {} type
  set static_ved(selectedtype) $type
}

proc ved_draw_tree {win} {
  global global_setup static_ved
  if {[info exists global_setup(vedtype_font)] == 0} {
    set global_setup(vedtype_font) {-*-courier-medium-r-*-*-12-*-*-*-*-*-*-*}
  }
  boxsize w h $win $global_setup(vedtype_font) {VED EventManager
      CrateController Chimaere Camac Fastbus CamacChimaere FastbusChimaere}
  incr w 4
  incr h 4
  set rand [expr $w/10]
  $win config -height [expr 10*$h+2*$rand] -width [expr 3*$w+4*$rand]
  for {set i 0} {$i<8} {incr i} {
    set box${i}(w) $w
    set box${i}(h) $h
  }
  set box0(x) [expr 2*$rand+$w]
  set box0(y) $rand
  set box1(x) [expr $rand+$w/2+$rand/2]
  set box1(y) [expr $rand+3*$h]
  set box2(x) [expr 2*$rand+$w+$w/2+$rand/2]
  set box2(y) [expr $rand+3*$h]
  set box3(x) $rand
  set box3(y) [expr $rand+6*$h]
  set box4(x) [expr 2*$rand+$w]
  set box4(y) [expr $rand+6*$h]
  set box5(x) [expr 3*$rand+2*$w]
  set box5(y) [expr $rand+6*$h]
  set box6(x) [expr $rand+$w/2+$rand/2]
  set box6(y) [expr $rand+9*$h]
  set box7(x) [expr 2*$rand+$w+$w/2+$rand/2]
  set box7(y) [expr $rand+9*$h]
  textbox $win box0 VED $global_setup(vedtype_font)\
    {type_any}
  textbox $win box1 EventManager $global_setup(vedtype_font)\
      {type_eventmanager}
  textbox $win box2 CrateController $global_setup(vedtype_font)\
      {type_controller DEF}
  textbox $win box3 Chimaere $global_setup(vedtype_font)\
      {type_chimaere}
  textbox $win box4 Camac $global_setup(vedtype_font)\
      {type_camac}
  textbox $win box5 Fastbus $global_setup(vedtype_font)\
      {type_fastbus}
  textbox $win box6 CamacChimaere $global_setup(vedtype_font)\
      {type_camac_chimaere}
  textbox $win box7 FastbusChimaere $global_setup(vedtype_font)\
      {type_fastbus_chimaere}
  connectboxes $win box0 box1
  connectboxes $win box0 box2
  connectboxes $win box1 box3
  connectboxes $win box2 box3
  connectboxes $win box2 box4
  connectboxes $win box2 box5
  connectboxes $win box3 box6
  connectboxes $win box3 box7
  connectboxes $win box4 box6
  connectboxes $win box5 box7
  $win bind BOTH <Enter> "any_enter $win"
  $win bind BOTH <Leave> "any_leave $win"
  $win bind BOTH <1> "entered $win"
}

proc ved_win_open {} {
  global static_ved win_pos

  if $win_pos(ved) then {wm positionfrom .ved user}
  ved_reset
  wm deiconify .ved
  set win_pos(ved) 1
}

proc create_ved_win {} {
  global global_setup static_ved win_pos

  set static_ved(name) {}
  set win_pos(ved) 0
  set self .ved
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] open ved"

  frame $self.typeframe -relief raised -borderwidth 1
  frame $self.nameframe -relief raised -borderwidth 1
  frame $self.listframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# typeframe
  label $self.typeframe.l -text "VEDtype:" -anchor w
  set static_ved(cv) [canvas $self.typeframe.c]
  ved_draw_tree $self.typeframe.c

  pack $self.typeframe.l -side top -padx 1m -fill x -expand 1
  pack $self.typeframe.c -side top -fill x -expand 1

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
  bind $self.listframe.b <KeyPress-Return> {
    set static_ved(name) [selection get]
  }
  bind $self.listframe.b <KeyPress-KP_Enter> {
    set static_ved(name) [selection get]
  }
  pack $self.listframe.l -side top -padx 1m -pady 1m -fill x -expand 0
  pack $self.listframe.b -side left -fill both -expand 1
  pack $self.listframe.s -side right -fill y
  set static_ved(listboxwidget) $self.listframe.b

# buttonframe
  button $self.b_frame.ok -text OK -command "ved_ok $self"
  button $self.b_frame.a -text Apply -command "ved_apply"
  button $self.b_frame.r -text Reset -command "ved_reset"
  button $self.b_frame.c -text Cancel -command "ved_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.r $self.b_frame.c\
      -side left -fill both -padx 1m -pady 1m -expand 1
#  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.c\
#      -side left -fill both -padx 1m -pady 1m -expand 1

  pack $self.typeframe -side top -fill x -expand 0
  pack $self.nameframe -side top -fill x -expand 0
  pack $self.listframe -side top -fill both -expand 1
  pack $self.b_frame -side bottom -fill x -expand 0
}
