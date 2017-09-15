# $ZEL: stop.tcl,v 1.15 2007/04/15 13:15:09 wuestner Exp $
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
  global clockformat

  set error 0

  if {($global_daq(status)!="running") && ($global_daq(status)!="pause")} {
    info_not_running_status
    return
  }

  output "STOP run $global_daq(run_nr)" time tag_under
  set global_daq(_stoptime) [clock seconds]

  change_status ? "stop in progress"
  map_status_display
  update idletasks

    if {[llength $global_setupdata(all_veds)]==1} {
        stop_daq_simple
    } else {
        stop_daq
    }

  change_status idle "initialized"
  set global_daq(stoptime) [clock format $global_daq(_stoptime) -format $clockformat]
  output "Stop of run $global_daq(run_nr) complete" tag_green
  run_nr::get_new_run_nr
}

# stop because of end of media or maximum filesize reached
# reason is (end_of_media|filesize_reached)
proc automatic_stop {ved do_idx reason} {
  global global_daq global_setupdata global_verbose
  global global_interps
  global clockformat

  if {($global_daq(status)!="running") && ($global_daq(status)!="pause")} {
    output "automatic stop ($reason); but DAQ is not running." tag_red
    return
  }

  output "Automatic STOP of run $global_daq(run_nr)" time tag_under
  set global_daq(_stoptime) [clock seconds]

  change_status ? "stop in progress"
  map_status_display
  update idletasks

  stop_daq

  change_status idle "initialized"
  set global_daq(stoptime) [clock format $global_daq(_stoptime) -format $clockformat]
  output "Automatic Stop of run $global_daq(run_nr) complete" tag_green
  run_nr::get_new_run_nr

    switch $reason {
    end_of_media {
        set space $global_interps($ved)
        if [$space eval info exists dataout_autochange] {
            $ved dataout enable $do_idx 0
            auto_restart $ved $do_idx $reason
        }

        output "ejecting old Tape"
        if [catch {
            $ved command1 TapeAllowRemoval $do_idx
            $ved command1 TapeUnload $do_idx 1
        } mist] {
            output "eject tape: $mist" tag_red
        }
    }
    filesize_reached {
            auto_restart $ved $do_idx $reason
    }
    default {
        output "automatic stop: reason \"$reason\" not valid" tag_red
    }
    }
}
