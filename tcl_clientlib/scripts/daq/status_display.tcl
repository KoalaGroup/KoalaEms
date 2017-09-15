# $ZEL: status_display.tcl,v 1.15 2010/09/09 23:16:38 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
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
# global_rostatvar_rate_$ved
#

proc create_status_display {} {
  global global_statwin

  set global_statwin(win) [toplevel .status -class VEDstatus]
  wm withdraw .status
  wm title .status {VED Status}
  set global_statwin(frame) [frame .status.f]
  wm protocol .status WM_DELETE_WINDOW {wm withdraw .status}
  wm positionfrom .status user
  wm sizefrom .status user
#   frame .status.bf
#   button .status.bf.u -text update
#   pack .status.bf.u -side top -expand 1 -fill x
  pack .status.f -side top
#   pack .status.bf -side top -expand 1 -fill x
}

proc create_status_frame {ved column row} {
  global global_statwin global_ved_descriptions
  global global_statframes global_statuslabel
  global entry_ro

  set f [frame $global_statwin(frame).$ved -relief ridge -borderwidth 3]
  set vedname [$ved name]
  label $f.l -text "$vedname ($global_ved_descriptions($vedname))" \
        -background #a0a0a0 -anchor w
  pack $f.l -side top -anchor w -fill x
  set global_statuslabel($ved) $f.l

  set global_statframes($ved) [frame $f.g]
  label $f.g.ro -text "readout:"
  global global_rostatvar_$ved
  global global_rostatvar_num_$ved
  global global_rostatvar_rate_$ved
  set global_rostatvar_${ved}(ro) ---
  set global_rostatvar_num_${ved}(ro) 0
  set global_rostatvar_rate_$ved 0

  entry $f.g.ro_s -relief flat -state $entry_ro\
      -textvariable global_rostatvar_${ved}(ro) -width 40

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
    entry $f.g.do_${i}_s -relief flat -state $entry_ro\
        -textvariable global_rostatvar_${ved}($i) -width 40
    grid configure $f.g.do_$i -column 0 -row $n -sticky e
    grid configure $f.g.do_${i}_s -column 1 -row $n -sticky ew
  }
  pack $f.g -side top -expand 1 -fill both
  #pack $f -side top -expand 1 -fill both
  grid $f -column $column -row $row
}

proc create_status_frames {} {
    global global_statwin global_setupdata

    foreach i [winfo children $global_statwin(frame)] {
        destroy $i
    }

    set nveds 0
    if [info exists global_setupdata(em_ved)] {
        foreach i $global_setupdata(em_ved) {
            incr nveds
        }
    }
    if [info exists global_setupdata(ccm_ved)] {
        foreach i $global_setupdata(ccm_ved) {
            incr nveds
        }
    }
    if [info exists global_setupdata(cc_ved)] {
        foreach i $global_setupdata(cc_ved) {
            incr nveds
        }
    }

    set vpc 12 ;# veds per column
    set columns [expr ($nveds+$vpc-1)/$vpc]
    set vpc [expr ($nveds+$columns-1)/$columns]

    set fcount 0
    if [info exists global_setupdata(em_ved)] {
        foreach i $global_setupdata(em_ved) {
            create_status_frame $i [expr $fcount/$vpc] [expr $fcount%$vpc]
            incr fcount
        }
    }
    if [info exists global_setupdata(ccm_ved)] {
        foreach i $global_setupdata(ccm_ved) {
            create_status_frame $i [expr $fcount/$vpc] [expr $fcount%$vpc]
            incr fcount
        }
    }
    if [info exists global_setupdata(cc_ved)] {
        foreach i [lsort $global_setupdata(cc_ved)] {
            create_status_frame $i [expr $fcount/$vpc] [expr $fcount%$vpc]
            incr fcount
        }
    }
}

proc map_status_display {} {
  global global_statwin
  wm deiconify $global_statwin(win)
}

proc change_status_vedreset {ved} {
  global global_statframes

  if {[info exists global_statframes($ved)]==0} return
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
  global entry_ro

  set f $global_statframes($ved)
  set n [expr $i+1]
  label $f.do_$i -text "dataout $i:"
  set global_rostatvar_${ved}($i) ---
  set global_rostatvar_num_${ved}($i) 0
  entry $f.do_${i}_s -relief flat -state $entry_ro\
      -textvariable global_rostatvar_${ved}($i) -width 40
  grid configure $f.do_$i -column 0 -row $n -sticky e
  grid configure $f.do_${i}_s -column 1 -row $n -sticky ew
}

proc change_status_dodelete {ved i} {
  global global_statframes
  set f $global_statframes($ved)

  grid forget $f.do_$i $f.do_${i}_s
  destroy $f.do_$i $f.do_${i}_s
}
