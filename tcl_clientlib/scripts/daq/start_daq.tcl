# $ZEL: start_daq.tcl,v 1.13 2009/04/01 16:29:58 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
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

proc print_dataoutfilename {ved} {
  if [catch {set dos [$ved namelist do]} mist] {
    output "get namelist do from $ved: $mist" tag_red
  } else {
    foreach do $dos {
      if [catch {set dout [$ved dataout upload $do]} mist] {
        output "dataout upload from $ved dataout $do: $mist" tag_red
      } else {
        if {[string equal [lindex $dout 3] file]} {
          if [catch {set fname [$ved command1 get_dataoutfile $do | string]} mist] {
            output "get_dataoutfile from $ved dataout $do: $mist" tag_red
          } else {
            output "$ved $do: filename is $fname"
          }
        }
      }
    }
  }
}

proc start_daq {} {
  global global_setupdata global_verbose global_daq

  if {[call_run_commands $global_setupdata(names) start 0]} {
    return -1
  }

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
      set_dataoutloginfo $i start
      print_dataoutfilename $i
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
      set_dataoutloginfo $i start
    }
    if [test_running $global_setupdata(cc_ved)] {return -1}
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
      set_dataoutloginfo $i start
    }
    if [test_running $global_setupdata(ccm_ved)] {return -1}
  }

  if {[call_run_commands $global_setupdata(names) poststart 0]} {
    return -1
  }

  return 0
}
