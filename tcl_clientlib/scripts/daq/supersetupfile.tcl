# $ZEL: supersetupfile.tcl,v 1.5 2003/02/04 19:28:00 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_setup(super_setup)
# win_pos($win)
#

#unnoetig?
# proc dummy_supersetupfile {} {}

namespace eval supersetupfile {

variable win .xh_supersetupfile
variable name

proc reset {} {
  global global_setup
  variable name
  set name $global_setup(super_setup)
}

proc ok {} {
  global global_setup
  variable win
  variable name
  set global_setup(super_setup) $name
  wm withdraw $win
}

proc cancel {} {
  variable win
  wm withdraw $win
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
  wm title $win "DAQ Super Setup File"
  wm maxsize $win 10000 10000

  label $win.h -relief raised -borderwidth 1 -text "Name of Super Setupfile:"
  entry $win.e -relief sunken -borderwidth 1\
    -textvariable [namespace current]::name
  frame $win.b -relief raised -borderwidth 1

  button $win.b.ok -text OK -command [namespace code ok]
  button $win.b.r -text Reset -command [namespace code reset]
  button $win.b.c -text Cancel -command [namespace code cancel]
  pack $win.b.ok $win.b.r $win.b.c\
      -side left -fill both -padx 2m -pady 2m -expand 1

  pack $win.h -side top -fill x -expand 1
  pack $win.b -side bottom -fill both -expand 1
  pack $win.e -side top -fill x -expand 1
}

}
