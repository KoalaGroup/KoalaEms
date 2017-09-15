# $ZEL: init.tcl,v 1.13 2009/04/01 16:29:58 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_daq(status)
#

proc ask_for_init {status} {
  set win .daq_question
  switch $status {
    idle {
      set text {DAQ is already initialized.\
        New initialisation will reset all VEDs}
      }
    running {
      set text {DAQ is running. Initialisation will stop the readout,\
        reset all VEDs and discard all data still not written to tape.}
      }
    pause {
      set text {DAQ is active. Initialisation will stop the readout,\
        reset all VEDs and discard all data still not written to tape.}
      }
    default {
      set text {DAQ status ist not known (programm error)}
      }
  }
  log_see_end
  set res [tk_dialog $win "DAQ Question" $text \
        question 1 {proceed with initialize} {abort initialize}]
  update idletasks
  return $res
}

proc ask_for_reset {} {
  set win .daq_question
  set text {The readout status of some or all VEDs is not "inactive" (see log).\
    Initialisation will stop the readout,\
    reset all VEDs and discard all data not written to tape yet. The tapefile\
    may be left in an unusable state.}
  log_see_end
  set res [tk_dialog $win "DAQ Question" $text \
        question 1 {proceed with initialize} {abort initialize}]
  update idletasks
  return $res
}

proc ask_for_rreset {} {
  set win .daq_question
  set text {The readout status of some or all VEDs is not "inactive" (see log).\
    Reset will stop the readout,\
    reset all VEDs, discard all data still not written to tape and kill the\
    operator.}
  log_see_end
  set res [tk_dialog $win "DAQ Question" $text \
        question 1 {proceed with reset} {abort reset}]
  update idletasks
  return $res
}

proc init_button {} {
  global global_daq
  output "INIT" time tag_under

  set last_stat $global_daq(status)
  set last_text $global_daq(statustext)
  set weiter 1
  if {![string equal $global_daq(status) "uninitialized"] &&
      ![string equal $global_daq(status) "idle"]} {
    set weiter [expr [ask_for_init $global_daq(status)]==0]
  }
  if {!$weiter} {
    output "Init aborted" tag_red
    return
  }

  stop_status_loop

  change_status uninitialized "Init in progress"
  # brutal, aber wirksam
  if [ems_connected] {
    if [catch ems_disconnect mist] {
      output $mist tag_red
    }
  update idletasks
#  after 2000
  }

  if [read_setup_data] {
    change_status uninitialized "Init not complete (can't read setup files)."
    return
  }

  if [open_veds] {
    change_status uninitialized "Init not complete (cannot open VEDs)."
    return
  }
  if [check_ved_status inactive] {
    set weiter [expr [ask_for_reset]==0]
  }

  if {!$weiter} {
    output "Init aborted" tag_red
    change_status $last_stat $last_text
    return
  }
  if [setup_veds] {
    change_status uninitialized "Init not complete (cannot setup VEDs)."
    return
  }
  headers::create_headers
  update idletasks
  map_status_display
  update idletasks
  start_status_loop
  update idletasks
  if {[init_periodic_procs]} {
    change_status uninitialized "Init not complete (cannot start periodic procedures)."
    return
  }
  change_status idle "initialized"
  output "Init complete" tag_green
  update idletasks
}

proc reset_button {} {
  global global_daq
  output "RESET" time tag_under

  set weiter 1

  if {$global_daq(status)=="uninitialized"} {
    if [read_setup_data] return
  }
  stop_status_loop
  if [open_veds] return

  if [check_ved_status inactive] {
    set weiter [expr [ask_for_rreset]==0]
  }
  if {!$weiter} {
    output "Reset aborted" tag_red
    return
  }

  change_status uninitialized "Reset in progress"

  if [reset_veds] {
    change_status uninitialized "reset not complete."
    return
  }

  change_status uninitialized "reset"
  output "Reset complete" tag_green
}
