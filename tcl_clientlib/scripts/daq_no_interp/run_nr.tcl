# $Id: run_nr.tcl,v 1.3 1999/04/09 12:32:40 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_setup(run_nr_file)
# global_daq(run_nr)        for display (may contain text also)
# global_daq(_run_nr)       for local caching
#

proc dummy_run_nr {} {}

namespace eval run_nr {
#namespace export get_new_run_nr run_nr_used

proc get_new_run_nr {} {
  global global_setup global_daq

  if [catch {set rfile [open $global_setup(run_nr_file) RDONLY]} mist] {
    output "cannot open file with last run number ($global_setup(run_nr_file)),\
      $global_daq(_run_nr) assumed" tag_red
  } else {
    if {[gets $rfile string]<0} {
      output "cannot read file with last run number\
        ($global_setup(run_nr_file)), $global_daq(_run_nr) assumed" tag_red
    } else {
      if {[scan $string {%d} nr]!=1} {
        output "cannot convert content of file with last run number\
          ($global_setup(run_nr_file)), $global_daq(_run_nr) assumed" tag_red
      } else {
        if {($nr!=$global_daq(_run_nr)) && ($global_daq(_run_nr)!=0)} {
          output "Somebody has changed the run number in\
            $global_setup(run_nr_file) from $global_daq(_run_nr) to $nr"\
            tag_orange
        }
        set global_daq(_run_nr) $nr
      }
    }
    close $rfile
  }
  set global_daq(run_nr) "$global_daq(_run_nr) (next run)"
  return $global_daq(_run_nr)
}

proc run_nr_used {} {
  global global_setup global_daq

  set global_daq(run_nr) "$global_daq(_run_nr)"
  incr global_daq(_run_nr)
  if [catch {set rfile [open $global_setup(run_nr_file) {WRONLY CREAT TRUNC}]} mist] {
    output "cannot open file with last run number for writing\
      ($global_setup(run_nr_file))" tag_red
  } else {
    puts $rfile $global_daq(_run_nr)
    close $rfile
  }
}

}
