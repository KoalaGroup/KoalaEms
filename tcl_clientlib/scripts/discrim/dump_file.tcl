# $Id: dump_file.tcl,v 1.1 2000/07/15 21:38:19 wuestner Exp $
#

proc dump_thresholds {} {
  global global_setup global_moduls

  set filename $global_setup(dumpfile)
  set file [open $filename {WRONLY CREAT}]

  foreach modul $global_setup(moduls) {
    set slot [lindex $modul 0]
    set type [lindex $modul 1]
    switch $type {
      4413 {dump_4413 $slot $file}
      C193 {dump_C193 $slot $file}
    }
  }
  close $file
}
