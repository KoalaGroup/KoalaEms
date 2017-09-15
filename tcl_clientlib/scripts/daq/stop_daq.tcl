# $ZEL: stop_daq.tcl,v 1.16 2009/06/05 15:46:56 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_verbose
# global_setupdata(all_veds)
# global_setupdata(ccm_ved)
# global_setupdata(cc_ved)
# global_setupdata(em_ved)
# global_veds_writing($ved)
# global_veds_reading($ved)
# global_veds_running($ved)
#

proc set_dataoutloginfo {ved st} {
# tsm_archive_ems expects the following line:
# yyyy-mm-dd_SS:MM:SS code filename run starttime stoptime events
# date, code and filename is delivered by the event manager
# we have to add: run starttime stoptime events

    global global_daq

    if [catch {set dos [$ved namelist do]} mist] {
        output "get namelist do from $ved: $mist" tag_red
        return
    }

    set line {}
    append line [run_nr::get_run_nr]
    append line " $global_daq(_starttime)"

    if {[string equal $st stop]} {
        set events [lindex [$ved readout stat] 1]
        append line " $global_daq(_stoptime)"
        append line " $events"
        output "last run([run_nr::get_run_nr]): \
start=$global_daq(_starttime) \
stop=$global_daq(_stoptime) \
events=$events" tag_blue
    }

    foreach do $dos {
        if [catch {$ved command1 set_dataoutloginfo $do '${line}'} mist] {
            output "$ved set_dataoutloginfo $do: $mist" tag_orange
        } else {
            #output "wrote $line to ved $ved; do $do"
        }
    }
}

proc stop_veds {veds} {
  global global_verbose

  set error 0
  foreach i $veds {
    if {$global_verbose} {
      output "resetting readout of $i" tag_green
    }
    set_dataoutloginfo $i stop
    if [catch {$i readout reset} mist] {
      output "reset readout on $i: $mist" tag_red
      set error -1
    }
  }
  return $error
}

proc pause_veds {veds} {
  global global_verbose

  foreach i $veds {
    if {$global_verbose} {
      output "stopping readout of $i" tag_green
    }
    if [catch {$i readout stop} mist] {
      output "stop readout on $i: $mist" tag_red
    }
  }
}

proc stop_daq {} {
  global global_setupdata global_verbose global_interps global_daq
  global global_veds_writing global_veds_reading global_veds_running

  ::headers::update_superheader_stop $global_daq(_stoptime)
#   output "all_veds: $global_setupdata(all_veds)"
#   output "ccm_ved : $global_setupdata(ccm_ved)"
#   output "cc_ved  : $global_setupdata(cc_ved)"
#   output "em_ved  : $global_setupdata(em_ved)"

  foreach i $global_setupdata(all_veds) {
    set global_veds_writing($i) $global_veds_running($i)
    set global_veds_reading($i) $global_veds_running($i)
  }

  set error 0
  set cc {}

  if {[call_run_commands $global_setupdata(names) prestop 0]} {
    return -1
  }

# stop triggermaster
  if {[llength $global_setupdata(ccm_ved)]>0} {
    if {$global_verbose} {
      output "resetting trigger master $global_setupdata(ccm_ved)" tag_green
    }
    pause_veds $global_setupdata(ccm_ved)
    after 500
    if [stop_veds $global_setupdata(ccm_ved)] {set error -1}
    if {!$error} {wait_for_stopped $global_setupdata(ccm_ved)}
    foreach i $global_setupdata(ccm_ved) {
      lappend cc $i
    }
  }

# stop crate controllers
  if {[llength $global_setupdata(cc_ved)]>0} {
    if {$global_verbose} {
      output "resetting crate controllers $global_setupdata(cc_ved)" tag_green
    }
    if [stop_veds $global_setupdata(cc_ved)] {set error -1}
    if {!$error} {wait_for_stopped $global_setupdata(cc_ved)}
    foreach i $global_setupdata(cc_ved) {
      lappend cc $i
    }
  }

  if {!$error} {set error [data_in_outs_usable 0]}
  if {!$error} {write_setup_to_dataouts $cc}
# wait for cc ready
  if {!$error && [llength $cc]>0} {
    output "wait for dataouts of [vedtextlist $cc] to become ready" tag_blue
    while {[llength $cc]>0} {
      set cc1 [wait_for_do_ready $cc 1]
      set cc [ldiff $cc $cc1]
    }
  }

  if {[llength $global_setupdata(em_ved)]>0} {
  # compute maximum eb-level and create a list for each level
    set maxlevel 1
    foreach ved $global_setupdata(em_ved) {
      set name $global_interps($ved)
      set level [$name eval set eventbuilder]
      if {$level>$maxlevel} {set maxlevel $level}
      lappend em_list($level) $ved
    }

  # iterate over all levels and stop eventmanagers
    for {set level 1} {$level<=$maxlevel} {incr level} {
      set em $em_list($level)
      if {!$error} {set error [data_in_outs_usable $level]}
      if {!$error} {
        output "wait for datains of [vedtextlist $em] to become ready" tag_blue
        while {[llength $em]>0} {
          set em1 [wait_for_di_ready $em 1]
          set em [ldiff $em $em1]
          if {$global_verbose} {
            output "resetting event manager $em1" tag_green
          }
          write_setup_to_dataouts $em1
          stop_veds $em1
        }
      } else {
        stop_veds $em
      }
      wait_for_stopped $em_list($level)
    # wait for em ready
      if {!$error} {set error [data_in_outs_usable $level]}
      if {!$error} {
        set em $global_setupdata(em_ved)
        output "wait for dataouts of [vedtextlist $em] to become ready" tag_blue
        while {[llength $em]>0} {
          set em1 [wait_for_do_ready $em 1]
          set em [ldiff $em $em1]
        }
      }
    }
  }

  if {[call_run_commands $global_setupdata(names) stop 1]} {
    return -1
  }

  return 0
}
