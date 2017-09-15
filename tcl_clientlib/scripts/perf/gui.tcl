proc start_gui {} {
  crate_main_win
#  update idletasks
#  wm deiconify .
}

proc crate_main_win {} {
  global global_dispwin global_setup

  menu .menubar
  set filemenu [create_menu_file .menubar]
  set setupmenu [create_menu_setup .menubar]
  set vedmenu [create_menu_ved .menubar]
  set helpmenu [create_menu_help .menubar]

  .menubar add cascade -label File -underline 0 -menu $filemenu
  .menubar add cascade -label Setup -underline 0 -menu $setupmenu
  .menubar add cascade -label VED -underline 0 -menu $vedmenu
  .menubar add cascade -label Help -underline 0 -menu $helpmenu

  frame .bar -height 4 -width 10c -background black
  set global_dispwin(frame) [frame .displayframe]
  Display::create_display .displayframe
  pack .bar .displayframe -side top -fill x
  pack .displayframe -side top -fill both -expand 1
  . configure -menu .menubar
}
