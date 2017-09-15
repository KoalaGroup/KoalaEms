proc unpack_all {} {
  pack forget .menu .bar .canvas .legend .scroll
  }

proc pack_all {} {
  global global_setup
  pack forget .menu .bar .canvas .legend .scroll
  if $global_setup(show_menubar) then { pack .menu -side top -fill x }
  pack .bar -side top -fill x
  if $global_setup(show_legend) then { pack .legend -side bottom -fill x }
  if $global_setup(show_scrollbar) then { pack .scroll -side bottom -fill x }
  pack .canvas -side top -expand 1 -fill both
  }
