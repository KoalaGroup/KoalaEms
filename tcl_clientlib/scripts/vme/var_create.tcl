#$Id: var_create.tcl,v 1.1 2000/07/15 21:33:34 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

proc dummy_var_insert {} {}

namespace eval var_insert {

variable win .vme_ved_create
variable name
variable addr
variable size

proc ok {} {
  global global_setup
  variable win
  variable name
  variable addr
  variable size

  varframe::create_varline $name $addr $size
  varframe::rearrange
  wm withdraw $win
}

proc cancel {} {
  variable win
  wm withdraw $win
}

proc reset {} {
  variable name
  variable addr
  variable size
  set name ""
  set addr ""
  set size 32
}

proc win_open {} {
  global win_pos
  variable win

  if {![winfo exists $win]} win_create
  if $win_pos($win) then {wm positionfrom $win user}
  reset
  wm deiconify $win
  set win_pos($win) 1
}

proc win_create {} {
  global win_pos
  variable win

  set win_pos($win) 0
  toplevel $win
  wm withdraw $win
  wm group $win .
  wm title $win "Create Variable"
  wm maxsize $win 10000 10000

  frame $win.v -relief raised -borderwidth 1
  label $win.v.ln -text "Name:"
  label $win.v.la -text "Address:"
  label $win.v.ls -text "Size:"
  entry $win.v.en -textvariable [namespace current]::name
  entry $win.v.ea -textvariable [namespace current]::addr
  menubutton $win.v.m -textvariable [namespace current]::size \
      -indicatoron 1 -menu $win.v.m.m -width 6\
      -relief raised -bd 2 -highlightthickness 0 -anchor c -pady -1p
  menu $win.v.m.m -tearoff 0
  foreach i {8 16 32 menu} {
    $win.v.m.m add command -label $i \
        -command [namespace code "set size $i"]
  }
  grid $win.v.ln $win.v.en
  grid $win.v.la $win.v.ea
  grid $win.v.ls $win.v.m -sticky we
  frame $win.b -relief raised -borderwidth 1

  button $win.b.ok -text OK -command [namespace code ok]
  button $win.b.c -text Cancel -command [namespace code cancel]
  pack $win.b.ok $win.b.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $win.b -side bottom -fill both -expand 1
  pack $win.v -side top -fill x -expand 1
}

}
