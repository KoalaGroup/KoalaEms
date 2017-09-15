proc start_gui {} {
  crate_main_win
  update idletasks
  wm deiconify .
}

proc crate_main_win {} {
  global global_dispwin global_setup

  frame .menuframe
  frame .bar -height 3 -background #f00
  set global_dispwin(frame) [frame .displayframe]
  frame .scaleframe
  create_scale .scaleframe

  set filemenu [create_menu_file .menuframe]
  set setupmenu [create_menu_setup .menuframe]
  set commenu [create_menu_commands .menuframe]
  set printmenu [create_menu_print .menuframe]
  pack $filemenu $setupmenu $commenu -side left -padx 1m

  pack_all
}

proc pack_all {} {
  global global_setup global_running

  pack .menuframe .bar -side top -fill x
  if {$global_setup(interval_slider)} {
    pack .scaleframe -side bottom -fill x
  }
  if {$global_running(numerical_display)} {
    pack .displayframe -side bottom -fill both -expand 1
  }
}

proc repack_all {} {
  pack forget .menuframe .bar .displayframe .scaleframe
  pack_all
}
