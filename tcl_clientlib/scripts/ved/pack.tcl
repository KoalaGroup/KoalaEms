proc pack_main {} {
  pack forget .menu .bar .canvas
  pack .menu -side top -fill x
  pack .bar -side top -fill x
  pack .status -side top -expand 1 -fill both
}
