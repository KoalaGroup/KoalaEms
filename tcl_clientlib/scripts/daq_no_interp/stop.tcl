# $Id: stop.tcl,v 1.9 2000/08/31 15:43:08 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_verbose
# global_daq(status)
# global_daq(_starttime)
# global_setupdata(em_ved)  ; Eventbuilder
# global_setupdata(ccm_ved) ; Cratecontroller mit Mastertrigger
# global_setupdata(cc_ved)  ; Cratecontroller
#

proc info_not_running_status {} {
  set win .daq_question
  set text {DAQ is not running. Please start it before stop.}
  tk_dialog $win "DAQ Message" $text warning 0 {OK}
}

proc stop_button {} {
  global global_daq global_setupdata global_verbose
  set error 0

  if {($global_daq(status)!="running") && ($global_daq(status)!="pause")} {
    info_not_running_status
    return
  }

  output "STOP" time tag_under
  set global_daq(_stoptime) [clock seconds]

  change_status ? "stop in progress"
  map_status_display
  update idletasks

  stop_daq

  change_status idle "initialized"
  set global_daq(stoptime) [clock format $global_daq(_stoptime)]
  output "Stop complete" tag_green
  run_nr::get_new_run_nr
}

proc stop_because_of_end_of_media {ved do_idx} {
  global global_daq global_setupdata global_verbose
  global global_namespaces

  if {($global_daq(status)!="running") && ($global_daq(status)!="pause")} {
    output "should stop because end of media reached; but DAQ is not running." tag_red
    return
  }

  output "Automatic STOP" time tag_under
  set global_daq(_stoptime) [clock seconds]

  change_status ? "stop in progress"
  map_status_display
  update idletasks

  stop_daq

  change_status idle "initialized"
  set global_daq(stoptime) [clock format $global_daq(_stoptime)]
  output "Automatic Stop complete" tag_green
  run_nr::get_new_run_nr

  set space $global_namespaces($ved)
  if [info exists ${space}::dataout_autochange] {
    $ved dataout enable $do_idx 0
    auto_restart $ved $do_idx
  }

  output "ejecting old Tape"
  if [catch {
    $ved command1 TapeAllowRemoval $do_idx
    $ved command1 TapeUnload $do_idx 1
  } mist] {
    output "eject tape: $mist" tag_red
  }
}
