# $Id: status_display.tcl,v 1.5 1999/08/07 20:35:48 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
# 
# global_statwin(win)
# global_statwin(frame)
# global_setupdata(em_ved)
# global_setupdata(ccm_ved)
# global_setupdata(cc_ved)
# global_ved_descriptions($vedname)
# global_statframes($ved)
# global_rostatvar_$ved($do)
# global_rostatvar_num_$ved($do)
#

proc create_status_display {} {
  global global_statwin

  set global_statwin(win) [toplevel .status -class VEDstatus]
  wm withdraw .status
  wm title .status {VED Status}
  set global_statwin(frame) [frame .status.f]
  wm protocol .status WM_DELETE_WINDOW {wm withdraw .status}
#   frame .status.bf
#   button .status.bf.u -text update
#   pack .status.bf.u -side top -expand 1 -fill x
  pack .status.f -side top
#   pack .status.bf -side top -expand 1 -fill x
}

proc create_status_frame {ved} {
  global global_statwin global_ved_descriptions
  global global_statframes global_statuslabel

  set f [frame $global_statwin(frame).$ved -relief ridge -borderwidth 3]
  set vedname [$ved name]
  label $f.l -text "$vedname ($global_ved_descriptions($vedname))"
  pack $f.l -side top
  set global_statuslabel($ved) $f.l

  set global_statframes($ved) [frame $f.g]
  label $f.g.ro -text "readout:"
  global global_rostatvar_$ved
  global global_rostatvar_num_$ved
  set global_rostatvar_${ved}(ro) ---
  set global_rostatvar_num_${ved}(ro) 0
  entry $f.g.ro_s -relief flat -state disabled\
      -textvariable global_rostatvar_${ved}(ro)

  grid configure $f.g.ro -column 0 -row 0 -sticky e
  grid configure $f.g.ro_s -column 1 -row 0 -sticky ew

  if [catch {set dos [$ved namelist do]} mist] {
    output "$ved namelist do: $mist" tag_red
    return
  }
  foreach i $dos {
    set n [expr $i+1]
    label $f.g.do_$i -text "dataout $i:"
    set global_rostatvar_${ved}($i) ---
    set global_rostatvar_num_${ved}($i) 0
    entry $f.g.do_${i}_s -relief flat -state disabled\
        -textvariable global_rostatvar_${ved}($i) -width 40
    grid configure $f.g.do_$i -column 0 -row $n -sticky e
    grid configure $f.g.do_${i}_s -column 1 -row $n -sticky ew
  }
  pack $f.g -side top -expand 1 -fill both
  pack $f -side top -expand 1 -fill both
}

proc create_status_frames {} {
  global global_statwin global_setupdata

  foreach i [winfo children $global_statwin(frame)] {
    destroy $i
  }

  if [info exists global_setupdata(em_ved)] {
    foreach i $global_setupdata(em_ved) {
      create_status_frame $i
    }
  }
  if [info exists global_setupdata(ccm_ved)] {
    foreach i $global_setupdata(ccm_ved) {
      create_status_frame $i
    }
  }
  if [info exists global_setupdata(cc_ved)] {
    foreach i $global_setupdata(cc_ved) {
      create_status_frame $i
    }
  }
}

proc map_status_display {} {
  global global_statwin
  wm deiconify $global_statwin(win)
}

proc change_status_vedreset {ved} {
  global global_statframes
  set f $global_statframes($ved)
  set rows [lindex [grid size $f] 1]
  for {set i 1} {$i<$rows} {incr i} {
    foreach n [grid slaves $f -row $i] {
      destroy $n
    }
  }
}

proc change_status_docreate {ved i} {
  global global_statframes
  global global_rostatvar_$ved
  global global_rostatvar_num_$ved

  set f $global_statframes($ved)
  set n [expr $i+1]
  label $f.do_$i -text "dataout $i:"
  set global_rostatvar_${ved}($i) ---
  set global_rostatvar_num_${ved}($i) 0
  entry $f.do_${i}_s -relief flat -state disabled\
      -textvariable global_rostatvar_${ved}($i)
  grid configure $f.do_$i -column 0 -row $n -sticky e
  grid configure $f.do_${i}_s -column 1 -row $n -sticky ew
}

proc change_status_dodelete {ved i} {
  global global_statframes
  set f $global_statframes($ved)

  grid forget $f.do_$i $f.do_${i}_s
  destroy $f.do_$i $f.do_${i}_s
}
