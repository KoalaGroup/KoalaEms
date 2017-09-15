# $ZEL: tape_display.tcl,v 1.21 2004/02/25 11:56:52 wuestner Exp $
# copyright: 1998, 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
# 
# global_tapestat(win)
# global_tapestat(list)
# global_tapestat_ved()
# global_tapestat_do()
# global_tapestat_var_host()
# global_tapestat_var_tape()
# global_tapestat_var_comp()
# global_tapestat_var_rewrites()
# global_tapestat_var_err_corr()
# global_tapestat_var_pos()
# global_tapestat_frame_pos()
# global_tapestat_frame_color
#

proc tapetype {ident} {
  switch -regexp -- $ident {
    TKZ09     -
    EXB8500.* -
    EXB-8500.* -
    EXB-8505.* {set type EXABYTE}
    DLT4000 -
    DLT4500 {set type DLT4000}
    DLT7000 {set type DLT7000}
    DLT8000 {set type DLT8000}
    default {
      output "tape $ident not known; cannot generate statistics" tag_red
      set type unknown
    }
  }
  return $type
}

proc create_tape_display {} {
  global global_tapestat

  set global_tapestat(win) [toplevel .tstatus -class TapeStatus]
  wm withdraw .tstatus
  wm title .tstatus {Tape Status}
  set global_tapestat(list) {}
  wm protocol .tstatus WM_DELETE_WINDOW {wm withdraw .tstatus}
}

proc map_tstatus_display {} {
  global global_tapestat
  wm deiconify $global_tapestat(win)
}

proc unmap_tstatus_display {} {
  global global_tapestat
  output "unmap_tstatus_display" tag_orange
  wm withdraw $global_tapestat(win)
}

proc add_DLT_to_display {f key tape_type} {
  global global_tapestat_frame_pos global_tapestat_frame_color
  global entry_ro

  #output "add_DLT_to_display($f $key $tape_type)"
  set is_7000 [regexp {DLT[78].*} $tape_type]
  set is_8000 [regexp {DLT8.*} $tape_type]
  #output "is_7000: $is_7000"
  if {$is_7000} {
    label $f.s.rem_l -text {Remaining Tape}
    entry $f.s.rem_e -relief sunken -state $entry_ro -justify right\
        -textvariable global_tapestat_var_rest($key)
    label $f.s.w_l -text {Estimated End of the World}
    entry $f.s.w_e -relief sunken -state $entry_ro -justify right\
        -textvariable global_tapestat_var_end($key)
  }
  label $f.s.h_l -text {Bytes transferred from Host}
  entry $f.s.h_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_host($key)
  label $f.s.a_l -text {Bytes/s}
  entry $f.s.a_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_rate($key)
  label $f.s.t_l -text {Bytes transferred to Tape}
  entry $f.s.t_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_tape($key)
  label $f.s.c_l -text {Compression Ratio}
  entry $f.s.c_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_comp($key)
  label $f.s.p_l -text {Position}
  entry $f.s.p_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_pos($key)
  if {$is_8000} {
    label $f.s.temp_l -text {Drive Temperature/°C}
    entry $f.s.temp_e -relief sunken -state $entry_ro -justify right\
        -textvariable global_tapestat_var_temp($key)
  }
  set global_tapestat_frame_pos($key) $f.s.p_e
  set global_tapestat_frame_color [$f.s.p_e cget -background]
  label $f.s.r_l -text {Rewrites}
  entry $f.s.r_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_rewrites($key)
#    label $f.s.e_l -text {Corrected errors}
#    entry $f.s.e_e -relief sunken -state $entry_ro -justify right\
#        -textvariable global_tapestat_var_err_corr($idx)
  set row 0
  if {$is_7000} {
    grid $f.s.rem_l -column 0 -row $row -sticky e
    grid $f.s.rem_e -column 1 -row $row -sticky ew
    incr row
    grid $f.s.w_l -column 0 -row $row -sticky e
    grid $f.s.w_e -column 1 -row $row -sticky ew
    incr row
  }
  grid $f.s.h_l -column 0 -row $row -sticky e
  grid $f.s.h_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.a_l -column 0 -row $row -sticky e
  grid $f.s.a_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.t_l -column 0 -row $row -sticky e
  grid $f.s.t_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.c_l -column 0 -row $row -sticky e
  grid $f.s.c_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.p_l -column 0 -row $row -sticky e
  grid $f.s.p_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.r_l -column 0 -row $row -sticky e
  grid $f.s.r_e -column 1 -row $row -sticky ew
  incr row
#    grid $f.s.e_l -column 0 -row 6 -sticky e
#    grid $f.s.e_e -column 1 -row 6 -sticky ew
  if {$is_8000} {
    grid $f.s.temp_l -column 0 -row $row -sticky e
    grid $f.s.temp_e -column 1 -row $row -sticky ew
    incr row
  }
}

proc add_EXA_to_display {f key tape_type} {
  global global_tapestat_frame_pos global_tapestat_frame_color
  global entry_ro

  output "add_EXA_to_display($f $key)"
  label $f.s.h_l -text {Remaining Tape (KByte)}
  entry $f.s.h_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_remaining($key)
  label $f.s.w_l -text {Estimated End of the World}
  entry $f.s.w_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_end($key)
  label $f.s.a_l -text {KByte/s}
  entry $f.s.a_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_rate($key)
  label $f.s.p_l -text {Position}
  entry $f.s.p_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_pos($key)
  set global_tapestat_frame_pos($key) $f.s.p_e
  set global_tapestat_frame_color [$f.s.p_e cget -background]
  label $f.s.r_l -text {Rewrites}
  entry $f.s.r_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_rewrites($key)
  set row 0
  grid $f.s.h_l -column 0 -row $row -sticky e
  grid $f.s.h_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.w_l -column 0 -row $row -sticky e
  grid $f.s.w_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.a_l -column 0 -row $row -sticky e
  grid $f.s.a_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.r_l -column 0 -row $row -sticky e
  grid $f.s.r_e -column 1 -row $row -sticky ew
  incr row
  grid $f.s.p_l -column 0 -row $row -sticky e
  grid $f.s.p_e -column 1 -row $row -sticky ew
}

proc add_UNKNOWN_to_display {f key tape_type} {
  global global_tapestat_frame_pos global_tapestat_frame_color
  global entry_ro

  output "add_UNKNOWN_to_display(... $key)"
  label $f.s.p_l -text {Position}
  entry $f.s.p_e -relief sunken -state $entry_ro -justify right\
      -textvariable global_tapestat_var_pos($key)
  set global_tapestat_frame_pos($key) $f.s.p_e
  set global_tapestat_frame_color [$f.s.p_e cget -background]
  grid $f.s.p_l -column 0 -row 0 -sticky e
  grid $f.s.p_e -column 1 -row 0 -sticky ew
}

proc add_tape_to_display {key} {
  global global_tapestat global_tapestat_ved global_tapestat_do
  global global_tapestat_frame
  global global_tapestat_var_host global_tapestat_var_tape
  global global_tapestat_var_host_letzt
  global global_tapestat_var_rate global_tapestat_var_time
  global global_tapestat_var_comp
  global global_tapestat_var_rewrites global_tapestat_var_err_corr
  global global_tapestat_var_pos global_tapestat_frame_pos
  global global_tapestat_var_temp
  global global_tapestat_frame_color
  global global_dataout_$key

  if {[lsearch -exact $global_tapestat(list) $key]!=-1} {
    #output "tape $key already in list, ignored"
    return
  }
  lappend global_tapestat(list) $key

  set global_tapestat_ved($key) [set global_dataout_${key}(ved)]
  set global_tapestat_do($key) [set global_dataout_${key}(idx)]
  set f [frame $global_tapestat(win).f_$key -relief ridge -borderwidth 3]
  set global_tapestat_frame($key) $f
  set global_tapestat_var_time($key) 0
  set global_tapestat_var_host($key) ---
  set global_tapestat_var_host_letzt($key) 0
  set global_tapestat_var_rate($key) ---
  set global_tapestat_var_tape($key) ---
  set global_tapestat_var_comp($key) ---
  set global_tapestat_var_rewrites($key) ---
  set global_tapestat_var_err_corr($key) ---
  set global_tapestat_var_pos($key) ---
  set global_tapestat_var_temp($key) ---
  init_tapeend $key
  frame $f.h
    frame $f.h.l -relief raised -borderwidth 3
      label $f.h.l.vl -text "VED: "
      label $f.h.l.vv -text "[$global_tapestat_ved($key) name]"
      label $f.h.l.dl -text "DO: "
      label $f.h.l.dv -text "$global_tapestat_do($key)"
      grid $f.h.l.vl $f.h.l.vv
      grid $f.h.l.dl $f.h.l.dv
      grid configure $f.h.l.vl $f.h.l.dl -sticky e
      grid configure $f.h.l.vv $f.h.l.dv -sticky w
    frame $f.h.r -relief raised -borderwidth 3
      label $f.h.r.o -text "Device: [set global_dataout_${key}(device)]"
      frame $f.h.r.u
        label $f.h.r.u.l -text [set global_dataout_${key}(tape)]
        label $f.h.r.u.r -text [set global_dataout_${key}(vendor)]
        pack $f.h.r.u.l $f.h.r.u.r -side left -fill x -expand 1
      pack $f.h.r.o $f.h.r.u -side top -fill x -expand 1
    pack $f.h.l -side left -fill both -expand 1
    pack $f.h.r -side left -fill x -expand 1

    frame $f.s -relief raised -borderwidth 3
  set tape_type [set global_dataout_${key}(type)]
  switch $tape_type {
    DLT4000 -
    DLT7000 {add_DLT_to_display $f $key $tape_type}
    DLT8000 {add_DLT_to_display $f $key $tape_type}
    EXABYTE {add_EXA_to_display $f $key $tape_type}
    default {
      output "tape [set global_dataout_${key}(tape)] unknown"
      add_UNKNOWN_to_display $f $key $tape_type
    }
  }
  pack $f.h $f.s -side top -fill x -expand 1
  pack $f -side top -fill x -expand 1
  wm deiconify $global_tapestat(win)
}

proc is_tape {domain} {
  set istape 0
  if {[llength $domain]<4} {
    set l [lindex $domain 1]
    if {[lindex $l 0]=="tape"} {set istape 1}
  } else {
    if {[lindex $domain 3]=="tape"} {set istape 1}
  }
  return $istape
}

proc add_do_to_tapedisplay {ved idx} {
  set key ${ved}_${idx}
  global global_dataout_$key
  set dom [$ved dataout upload $idx]
  if {[is_tape $dom]} {
    if [catch {set type [$ved command1 TapeInquiry $idx | stringlist]} mist] {
      output "[$ved name]: TapeInquiry $idx: $mist" tag_red
      set global_dataout_${key}(istape) 0
    } else {
    #output "TapeInquiry: $type"
    set global_dataout_${key}(tape) [lindex $type 0]
    set global_dataout_${key}(vendor) [lindex $type 1]
    set global_dataout_${key}(device) [lindex $dom 4]
    set global_dataout_${key}(type) [tapetype [lindex $type 0]]
#     output "tape: [set global_dataout_${key}(tape)]"
#     output "vendor: [set global_dataout_${key}(vendor)]"
#     output "device: [set global_dataout_${key}(device)]"
#     output "ved $ved dataout $idx is a tape; [lindex $type 1]"
    set global_dataout_${key}(istape) 1
    add_tape_to_display $key
    }
  } else {
    #output "add_do_to_tapedisplay: ved $ved dataout $idx is not a tape; ignored"
    set global_dataout_${key}(istape) 0
  }
}

proc delete_do_from_tapedisplay {ved idx} {
  global global_tapestat_frame global_tapestat
  set key ${ved}_${idx}
  global global_dataout_$key
  if [set global_dataout_${key}(istape)] {
    destroy $global_tapestat_frame($key)
    set x [lsearch -exact $global_tapestat(list) $key]
    if {$x>=0} {
      set global_tapestat(list) [lreplace $global_tapestat(list) $x $x]
    } else {
      output "delete_do_from_tapedisplay: $key does not exist in global_tapestat(list)"
    }
    if {[llength $global_tapestat(list)]==0} {unmap_tstatus_display}
  } else {
    #output "delete_do_from_tapedisplay: ved $ved dataout $idx is not a tape; ignored"
  }
}

proc set_tapedisplay_disabled {ved idx dis} {
  global global_tapestat_frame global_tapestat_frame_color
  set key ${ved}_${idx}
  global global_dataout_$key

  set frame $global_tapestat_frame($key)
  if {$dis} {
    set color #ff6000
  } else {
    set color $global_tapestat_frame_color
  }
  foreach w {h.l h.l.vl h.l.vv h.l.dl h.l.dv h.r h.r.o h.r.u h.r.u.l h.r.u.r} {
    ${frame}.${w} configure -background $color
  }
}

proc xformat {v} {
  set s {}
  while {$v>999} {
    set a [expr $v%1000]
    set v [expr $v/1000]
    set s [format {,%03d%s} $a $s]
  }
  set s [format {%d%s} $v $s]
  return $s
}

proc xfformat {v} {
  set s {}
  while {$v>999} {
    set a [expr int(fmod($v, 1000))]
    set v [expr ($v-$a)/1000]
    set s [format {,%03d%s} $a $s]
  }
  set s [format {%d%s} [expr int($v)] $s]
  return $s
}
