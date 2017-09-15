# $Id: start_gui.tcl,v 1.1 2000/03/24 17:13:23 wuestner Exp $

proc start_gui {} {
  global global_setup
  global global_moduls

  mainmenu

  set global_moduls {}
  foreach modul $global_setup(moduls) {
    set slot [lindex $modul 0]
    set type [lindex $modul 1]
    switch $type {
      4413 {create_4413_win $slot}
      C193 {create_C193_win $slot}
    }
  }

   foreach modul $global_moduls {
    modul_init $modul
  }
}
