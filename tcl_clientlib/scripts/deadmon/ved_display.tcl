# $Id: ved_display.tcl,v 1.1 2000/07/15 21:37:02 wuestner Exp $
# copyright:
# 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
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
  global global_perfframe
  global global_const_text

  set frame [frame $global_perfframe.$ved -relief ridge -borderwidth 4]
  set tframe [frame $frame.t]
  label $tframe.name -text "VED $name" -anchor w
  label $tframe.trate_l -text "Triggered Rate" -anchor w
  entry $tframe.trate_v1 -textvariable [namespace current]::trate \
      -justify right -state disabled -relief flat
  entry $tframe.trate_u1 -textvariable global_const_text(s) \
      -state disabled -relief flat -width 10
  label $tframe.mrate_l -text "Measured Rate" -anchor w
  entry $tframe.mrate_v1 -textvariable [namespace current]::mrate \
      -justify right -state disabled -relief flat
  entry $tframe.mrate_u1 -textvariable global_const_text(s) \
      -state disabled -relief flat -width 10
  label $tframe.dt_l -text "Dead Time" -anchor w
  entry $tframe.dt_v1 -textvariable [namespace current]::deadtime_ms \
      -justify right -state disabled -relief flat
  entry $tframe.dt_u1 -textvariable global_const_text(ms) \
      -state disabled -relief flat -width 10
  entry $tframe.dt_v2 -textvariable [namespace current]::deadtime_perc \
      -justify right -state disabled -relief flat
  entry $tframe.dt_u2 -textvariable global_const_text(percent) \
      -state disabled -relief flat -width 10
  grid $tframe.name -sticky ew
  grid $tframe.trate_l $tframe.trate_v1 $tframe.trate_u1 -sticky ew
  grid $tframe.mrate_l $tframe.mrate_v1 $tframe.mrate_u1 -sticky ew
  grid $tframe.dt_l $tframe.dt_v1 $tframe.dt_u1 $tframe.dt_v2 $tframe.dt_u2 \
      -sticky ew

  set dtframe [frame $frame.dt_disp -relief ridge -borderwidth 2]
  label $dtframe.name -text "Deadtime Distribution" -anchor w
  set dtc [canvas $dtframe.canvas -height 100p]
  pack $dtframe.name -side top -fill x
  pack $dtc -side top -fill x
  set trframe [frame $frame.trig_disp -relief ridge -borderwidth 2]
  label $trframe.name -text "Trigger Distribution" -anchor w
  set trc [canvas $trframe.canvas -height 100p]
  pack $trframe.name -side top -fill x
  pack $trc -side top -fill x

  pack $frame.t -side top -fill x
  pack $frame.dt_disp -side top -fill x
  pack $frame.trig_disp -side top -fill x

  pack $frame -side top -fill x
}
}

proc remove_ved_from_display {space} {
namespace eval $space {
  global global_perfframe
  destroy $frame
}
}
