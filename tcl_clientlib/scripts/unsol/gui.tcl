proc start_gui {} {
  crate_main_win
#  update idletasks
#  wm deiconify .
}

proc update_bar {name1 name2 op} {
  global global_main_loop
  if {$global_main_loop(running)} {
    .bar configure -background blue
  } else {
    .bar configure -background red
  }
}

proc crate_main_win {} {
  global global_setup global_main_loop

  menu .menubar
  set filemenu [create_menu_file .menubar]
  set setupmenu [create_menu_setup .menubar]
  set vedmenu [create_menu_ved .menubar]
  set actionmenu [create_menu_action .menubar]
  set helpmenu [create_menu_help .menubar]

  .menubar add cascade -label File -underline 0 -menu $filemenu
  .menubar add cascade -label Setup -underline 0 -menu $setupmenu
  .menubar add cascade -label VED -underline 0 -menu $vedmenu
  .menubar add cascade -label Actions -underline 0 -menu $actionmenu
  .menubar add cascade -label Help -underline 0 -menu $helpmenu

  frame .bar -height 3 -width 10c -background red
  if {$global_main_loop(running)} {
    .bar configure -background blue
  }
  trace variable global_main_loop(running) w update_bar
  create_log .logframe
  pack .bar -side top -fill x
  pack .logframe -side top -fill both -expand 1
  . configure -menu .menubar
}
