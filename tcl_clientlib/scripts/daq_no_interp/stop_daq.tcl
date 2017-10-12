# $Id: stop_daq.tcl,v 1.6 1999/08/29 19:24:26 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
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

proc stop_veds {veds} {
  global global_verbose

  set error 0
  foreach i $veds {
    if {$global_verbose} {
      output "resetting readout of $i" tag_green
    }
    if [catch {$i readout reset} mist] {
      output "reset readout on $i: $mist" tag_red
      set error -1
    }
  }
  return $error
}

proc stop_daq {} {
  global global_setupdata global_verbose global_namespaces
  global global_veds_writing global_veds_reading global_veds_running

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

# stop triggermaster
  if {[llength $global_setupdata(ccm_ved)]>0} {
    if {$global_verbose} {
      output "resetting trigger master $global_setupdata(ccm_ved)" tag_green
    }
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
  if {!$error} {write_setup_to_dataouts stop $cc}
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
      set name $global_namespaces($ved)
      set level [set ${name}::eventbuilder]
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
          write_setup_to_dataouts stop $em1
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

  foreach i $global_setupdata(names) {
    if [call_run_commands $i stop] {set error -1}
  }
  return 0
}
