# $ZEL: stop_daq_simple.tcl,v 1.3 2009/02/05 19:39:54 wuestner Exp $
# copyright:
# 2003 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc stop_daq_simple {} {
    global global_setupdata global_verbose global_interps global_daq
    global global_veds_writing global_veds_reading global_veds_running

    ::headers::update_superheader_stop $global_daq(_stoptime)

    set ved [lindex $global_setupdata(all_veds) 0]

    set global_veds_writing($ved) 1
    set global_veds_reading($ved) 1

    set error 0

    if {$global_verbose} {
        output "stopping readout of $ved" tag_green
    }
    if [catch {$ved readout stop} mist] {
      output "stop readout on $ved: $mist" tag_red
      set error -1
    }
    if {!$error} {
        set error [data_in_outs_usable 0]
    }
    if {!$error} {
        write_setup_to_dataouts $ved

        if {$global_verbose} {
            output "resetting readout of $ved" tag_green
        }
        if [catch {$ved readout reset} mist] {
          output "reset readout on $ved: $mist" tag_red
          set error -1
        }
        wait_for_do_ready $ved [expr !$global_verbose]
    }

    if {[call_run_commands $global_setupdata(names) stop 1]} {
       return -1
    }
    return 0
}
