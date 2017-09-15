# $Id: mainmenu.tcl,v 1.1 2000/03/24 17:13:22 wuestner Exp $

proc mainmenu {} {

  menu .menubar
  set filemenu [create_menu_file .menubar]
  .menubar add cascade -label File -underline 0 -menu $filemenu

  frame .bar -height 4 -width 2c -background VioletRed
  . configure -menu .menubar
  pack .bar -side top -fill x
}
