# $Id: start.tcl,v 1.8 2000/08/10 09:26:48 wuestner Exp $
# copyright: 1998, 1999
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_daq(status)
# global_daq(_starttime)
# global_init(data_read, init_done)
# global_setupdata(all_veds); alle VEDs
#

proc ask_for_start_wo_init {} {
  set win .daq_question
  set text {Initialisation has not been called. Start without Init will\
    probably lead to something not expected.}
  log_see_end
  set res [tk_dialog $win "DAQ Question" $text \
        questhead 1 {proceed with start} {abort start}]
  update idletasks
  return $res
}

proc ask_for_start_wrong_status {status} {
  set win .daq_question
  set text "DAQ status ($status) ist not expected (programm error).\
      Please use INIT button"
  tk_dialog $win "DAQ Message" $text warning 0 {OK}
  return 1
}

proc info_running_status {status} {
  set win .daq_question
  switch $status {
    running {
      set text {DAQ is running. Please stop it before restart.}
      }
    pause {
      set text {DAQ is active. Please stop it before restart.}
      }
    default {
      set text "DAQ status ($status) ist not expected (programm error).\
        Please use INIT button"
      }
  }
  tk_dialog $win "DAQ Message" $text warning 0 {OK}
}

proc start_button {} {
  global global_daq global_init global_setupdata

  if {($global_daq(status)=="running") || ($global_daq(status)=="pause")} {
    info_running_status $global_daq(status)
    return
  }

  output "START" time tag_under
  set global_daq(starttime) {}
  set global_daq(stoptime) {}
  set weiter 1

  change_status ? "Start in progress"
  map_status_display
  update idletasks

  if {!$global_init(data_read)} {
    if [read_setup_data] {
      change_status uninitialized "Start not done (can't read setup files)."
      return
    }
  }
  if [open_veds] {
    change_status uninitialized "Start not done (cannot open VEDs)."
    return
  }
  if {$global_daq(status)=="uninitialized"} {
    set weiter [expr [ask_for_start_wo_init]==0]
  } elseif {$global_daq(status)!="idle"} {
    set weiter [expr [ask_for_start_wrong_status $global_daq(status)]==0]
  }
  if {!$weiter} {
    output "Start aborted" tag_red
    change_status ? "Start aborted"
    return
  }

  if [select_initial_dataouts] {
    change_status ? "Start not complete."
    return
  }

  set global_daq(_starttime) [clock seconds]
  run_nr::get_new_run_nr
  ivalidate_headers
  set res [write_setup_to_dataouts start $global_setupdata(all_veds)]
  run_nr::run_nr_used
  if {$res!=0} {
    change_status ? "Start not complete."
    return
  }

  if [start_daq] {
    change_status ? "Start not complete."
    return
  }
  change_status running "Running"
  set global_daq(starttime) [clock format $global_daq(_starttime)]
  set global_daq(stoptime) {}
  output "Start complete" tag_green
  start_status_loop
}

proc auto_restart {ved do_idx} {
  global global_daq global_init global_setupdata

  if {$global_daq(status)!="idle"} {
    output "automatic restart; but DAQ is not idle." tag_red
    return
  }

  output "Automatic RESTART" time tag_under
  set global_daq(starttime) {}
  set global_daq(stoptime) {}
  set weiter 1

  change_status ? "Restart in progress"
  map_status_display
  update idletasks

  if [select_new_dataout $ved $do_idx] {
    change_status ? "Restart not complete."
    return
  }

  set global_daq(_starttime) [clock seconds]
  run_nr::get_new_run_nr
  ivalidate_headers
  
  set res [write_setup_to_dataouts restart $global_setupdata(all_veds)]
  run_nr::run_nr_used
  if {$res!=0} {
    change_status ? "Restart not complete."
    return
  }

  if [start_daq] {
    change_status ? "Restart not complete."
    return
  }
  change_status running "Running"
  set global_daq(starttime) [clock format $global_daq(_starttime)]
  set global_daq(stoptime) {}
  output "Start complete" tag_green
  start_status_loop
}
