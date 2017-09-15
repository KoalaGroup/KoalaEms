# $ZEL: start.tcl,v 1.19 2009/05/25 17:11:06 wuestner Exp $
# copyright: 1998, 1999
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_daq(status)
# global_daq(_starttime)
# global_setupdata(all_veds); alle VEDs
#

proc info_running_status {status} {
  set win .daq_question
  switch $status {
    running {
      set text {DAQ is running. Please stop it before restart.}
      }
    pause {
      set text {DAQ is active. Please stop it before restart.}
      }
    uninitialized {
      set text {DAQ is not initialized. Please use INIT button.}
      }
    default {
      set text "DAQ status ($status) ist not expected (programm error).\
        Please use INIT button."
      }
  }
  tk_dialog $win "DAQ Message" $text warning 0 {OK}
}

proc start_button {} {
  global global_daq global_setupdata
  global clockformat

  if {$global_daq(status)!="idle"} {
    info_running_status $global_daq(status)
    return
  }

  output "START run $global_daq(_run_nr)" time tag_under
  set global_daq(starttime) {}
  set global_daq(stoptime) {}
  set weiter 1

  change_status ? "Start in progress"
  map_status_display
  update idletasks

  if [open_veds] {
    change_status uninitialized "Start not done (cannot open all VEDs)."
    return
  }

  set global_daq(_starttime) [clock seconds]
  set global_daq(_stoptime) 0
  set runnr [run_nr::get_new_run_nr]

  foreach i $global_setupdata(names) {
    set_run_nr $i $runnr
  }

  if [select_initial_dataouts] {
    change_status ? "Start not complete."
    return
  }

  #::headers::create_noteheader
  ::headers::update_superheader_start $global_daq(_run_nr) $global_daq(_starttime)
  set res [write_setup_to_dataouts $global_setupdata(all_veds)]
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
  set global_daq(starttime) [clock format $global_daq(_starttime) -format $clockformat]
  set global_daq(stoptime) {}
  output "Start of run $global_daq(run_nr) complete" tag_green
  start_status_loop
}

# reason is (end_of_media|filesize_reached)
proc auto_restart {ved do_idx reason} {
  global global_daq global_setupdata
  global clockformat

  if {$global_daq(status)!="idle"} {
    output "automatic restart; but DAQ is not idle." tag_red
    return
  }

  if {[info exists global_daq(no_auto_restart)] && $global_daq(no_auto_restart)} {
    output {no automatic restart because it is disabled} tag_green
    set global_daq(no_auto_restart) 0
    return
  }

  output "Automatic RESTART" time tag_under
  set global_daq(starttime) {}
  set global_daq(stoptime) {}
  set weiter 1

  change_status ? "Restart in progress"
  map_status_display
  update idletasks

    if [string equal $reason "end_of_media"] {
        if [select_new_dataout $ved $do_idx] {
            change_status ? "Restart not complete."
            return
        }
    }

  set global_daq(_starttime) [clock seconds]
  set global_daq(_stoptime) 0
  set runnr [run_nr::get_new_run_nr]

  foreach i $global_setupdata(names) {
    set_run_nr $i $runnr
  }

  ::headers::update_superheader_start $global_daq(_run_nr) $global_daq(_starttime)
  set res [write_setup_to_dataouts $global_setupdata(all_veds)]
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
  set global_daq(starttime) [clock format $global_daq(_starttime) -format $clockformat]
  set global_daq(stoptime) {}
  output "Restart complete" tag_green
  start_status_loop
}

proc disable_auto_restart {} {
    global global_daq
    set global_daq(no_auto_restart) 1
    output {automatic restart is disabled} tag_red
}
