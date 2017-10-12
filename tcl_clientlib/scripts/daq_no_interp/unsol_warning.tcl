# $Id: unsol_warning.tcl,v 1.1 2000/08/31 15:43:12 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc unsol_warning {space v h d} {
  output "VED [$v name] ([set vedsetup_${space}::description]): warning"  tag_orange
  output_append "   data: $d"  tag_orange
  
  set code [lindex $d 0]
  set length [lindex $d 1] ; # ignored
  switch $code {
    0 {
      set evt [lindex $d 2]
      output_append "    no trigger detected (pedestal measurement), event $evt" tag_orange
    }
    1 {
      set evt [lindex $d 2]
      if {[llength $d]==5} {
        output_append "    trigger for pedestal measurement recorded (einer zu viel?) (event $evt)" tag_orange
      } else {
        set modultype [lindex $d 3]
        set con_pat [lindex $d 4]
        output_append "    time elapsed: conversion not completed for modul [format {0x%08x} $modultype]" tag_orange
        output_append "    modul: [modulname $modultype]" tag_orange
      }
    }
    2 {
      set evt [lindex $d 2]
      set pa [lindex $d 3]
      set ped [lindex $d 4]
      set errcode [lindex $d 5]
      output_append "    error in data word (pedestal measurement)" tag_orange
      output_append "    event $evt, slot $pa, data=[format {0x%x} $ped], errorcode $errcode" tag_orange
      set type [[set vedsetup_${space}::ved] command1 FRC $pa 0]
      output_append "    modul: [fbmodulname $type]" tag_orange
    }
    4 {
      set evt [lindex $d 2]
      set pa [lindex $d 3]
      set ptr1 [lindex $d 4]
      set ptr2 [lindex $d 5]
      output_append "    pointer mismatch in slot $pa: $ptr1 <--> $ptr2 (event $evt)" tag_orange
      set type [[set vedsetup_${space}::ved] command1 FRC $pa 0]
      output_append "    modul: [fbmodulname $type]" tag_orange
    }
    5 {
      set evt [lindex $d 2]
      set pa [lindex $d 3]
      set errcode [lindex $d 4]
      output_append "    multiple gates occured" tag_orange
      output_append "    event $evt, slot $pa, errorcode $errcode" tag_orange
      set type [[set vedsetup_${space}::ved] command1 FRC $pa 0]
      output_append "    modul: [fbmodulname $type]" tag_orange
      
    }
    6 {
      set pa [lindex $d 2]
      set sa [lindex $d 3]
      set val [lindex $d 4]
      output_append "    active channel depth wrong (??)" tag_orange
      output_append "    pa $pa, sa $sa val $val" tag_orange
      set type [[set vedsetup_${space}::ved] command1 FRC $pa 0]
      output_append "    modul: [fbmodulname $type]" tag_orange
      
    }
    7 {
      set modultype [lindex $d 2]
      output_append "    empty pattern for modultype [format {0x%08x} $modultype]" tag_orange
      output_append "    modul: [modulname $modultype]" tag_orange
    }
    8 {
      set evt [lindex $d 2]
      set modultype [lindex $d 3]
      set wc [lindex $d 4]
      output_append "    sscode==0 (too many data)" tag_orange
      output_append "    modul [format {0x%08x} $modultype], event $evt" tag_orange
      output_append "    modul: [modulname $modultype]" tag_orange
    }
    default {
      output_append "    unknown code $code" tag_orange
    }
  }
}

