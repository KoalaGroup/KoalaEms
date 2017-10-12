# $Id: start_daq.tcl,v 1.4 1999/04/09 12:32:42 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
# 
# global_verbose
# global_setupdata(names)
# global_setupdata(em_ved)
# global_setupdata(cc_ved)
# global_setupdata(ccm_ved)
#

proc test_running {veds} {
  set res 0
  foreach i $veds {
    if [catch {set stat [$i readout status]} mist] {
      output "[$i name]: get readoutstatus: $mist" tag_red
      set res 1
    }
    if {[lindex $stat 0]!="running"} {
      output "[$i name]: readoutstatus [lindex $stat 0], not running" tag_red
      set res 1
    }
  }
  return $res
}

proc start_daq {} {
  global global_setupdata global_verbose

  set res 0
  foreach i $global_setupdata(names) {
    if [call_run_commands $i start] {set res -1}
  }
  if {$res} {return -1}

# start eventbuilder
  if [info exists global_setupdata(em_ved)] {
    foreach i $global_setupdata(em_ved) {
      if {$global_verbose} {
        output "starting eventbuilder $i" tag_green
      }
      if [catch {$i readout start} mist] {
        output "start readout on $i: $mist" tag_red
        return -1
      }
    }
    if [test_running $global_setupdata(em_ved)] {return -1}
  }
# start all crate controllers but not triggermaster
  if [info exists global_setupdata(cc_ved)] {
    foreach i $global_setupdata(cc_ved) {
      if {$global_verbose} {
        output "starting crate controller $i" tag_green
      }
      if [catch {$i readout start} mist] {
        output "start readout on $i: $mist" tag_red
        return -1
      }
    }
    if [test_running $global_setupdata(em_ved)] {return -1}
  }
# start triggermaster
  if [info exists global_setupdata(ccm_ved)] {
    foreach i $global_setupdata(ccm_ved) {
      if {$global_verbose} {
        output "starting trigger master $i" tag_green
      }
      if [catch {$i readout start} mist] {
        output "start readout on $i: $mist" tag_red
        return -1
      }
    }
    if [test_running $global_setupdata(em_ved)] {return -1}
  }
  return 0
}
