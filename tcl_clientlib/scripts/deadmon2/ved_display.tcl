# $ZEL: ved_display.tcl,v 1.1 2000/07/15 21:37:02 wuestner Exp $
# copyright:
# 1999 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global_setup
# global_perfframe
#
proc init_const_text {} {
  global global_const_text

  set global_const_text(s) {/s}
  set global_const_text(ms) {ms}
  set global_const_text(percent) {%}
}

proc add_ved_to_display {space} {
namespace eval $space {
  global global_pframe_grid

  label $global_pframe_grid.name_$ved -text "VED $name" -anchor w
  entry $global_pframe_grid.ldt_$ved -textvariable [namespace current]::ldt \
      -justify right -state disabled -relief flat
  grid $global_pframe_grid.name_$ved $global_pframe_grid.ldt_$ved -sticky ew
}
}

proc remove_ved_from_display {space} {
namespace eval $space {
  #global global_perfframe
  #destroy $frame
}
}
