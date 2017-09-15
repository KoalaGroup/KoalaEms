# $ZEL: open_ved.tcl,v 1.21 2010/06/08 15:16:37 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_verbose
# global_setupdata(names)
# global_veds(...)
# global_veds_running(...)
# global_veds_writing(...)
# global_veds_reading(...)
# global_ved_descriptions(...)
# global_setupdata(all_veds); alle VEDs
# global_setupdata(em_ved)  ; Eventbuilder
# global_setupdata(ccm_ved) ; Cratecontroller mit Mastertrigger
# global_setupdata(cc_ved)  ; Cratecontroller
# global_interps($ved)      ; interpreters fuer alle veds
#

proc open_veds {} {
  global global_setupdata global_verbose global_ved_descriptions global_veds
  global global_veds_running global_veds_writing global_veds_reading
  global global_interps global_dataoutlist

  if {![ems_connected]} {
    output "try connect to commu" tag_orange
    if [connect_commu] {return -1}
    output_append "...OK" tag_orange
  }

  set global_setupdata(all_veds) {}
  set global_setupdata(em_ved) {}
  set global_setupdata(ccm_ved) {}
  set global_setupdata(cc_ved) {}
  set global_setupdata(other_ved) {}
  if [info exists global_veds] {unset global_veds}
  set openveds [ems_openveds]
  set num 0
  #output "names: $global_setupdata(names)"
  foreach i $global_setupdata(names) {
    #output "name: $i"
    set vedname [ved_setup_$i eval set vedname]
    #output_append "vedname: $vedname"
    set global_ved_descriptions($vedname) [ved_setup_$i eval set description]
    set ved ""
    #output "openveds=$openveds"
    foreach j $openveds {
      if {[string equal [$j name] $vedname]} {
        #output_append "true"
        set ved $j
        break
      } else {
        #output_append "false"
      }
    }
    if {$ved==""} {
      if {!$num} {output "open VEDs:"}
      incr num
      output_append "  $vedname"
      if [catch {set ved [ems_open $vedname]} mist] {
        output $mist tag_red
        return -1
      }
      $ved unsol "unsol_any $i %v %h %d"
      $ved typedunsol RuntimeError  "unsol_runtimeerror $i %v %h %d"
      $ved typedunsol Warning       "unsol_warning $i %v %h %d"
      $ved typedunsol Patience      "unsol_patience $i %v %h %d"
      $ved typedunsol StatusChanged "unsol_statuschanged $i %v %h %d"
      $ved typedunsol ActionCompleted "unsol_actioncompleted $i %v %h %d"
      $ved typedunsol DeviceDisconnect "unsol_devicedisconnect $i %v %h %d"
      $ved typedunsol Text          "unsol_text $i %v %h %d"
      $ved typedunsol Alarm         "unsol_alarm $i %v %h %d"
      #output "set global_dataoutlist($ved) {}"
      set global_dataoutlist($ved) {}
    } elseif {$global_verbose} {
      if {!$num} {output "open VEDs:"}
      incr num
      output_append "  $vedname already open"
    }
    set global_interps($ved) ved_setup_$i
    set global_veds($vedname) $ved
    #interp alias ved_setup_$i ved {} $ved
    ved_setup_$i alias ved $ved
    ved_setup_$i alias create_is sandbox_ved_is_create $ved ved_setup_$i

    #output "add $ved to all_ved"
    lappend global_setupdata(all_veds) $ved
    #output_append "all_ved: $global_setupdata(all_veds)"
    set global_veds_running($ved) 0
    set global_veds_writing($ved) 0
    set global_veds_reading($ved) 0
    if {[ved_setup_$i eval info exists eventbuilder] &&
        [ved_setup_$i eval set eventbuilder]} {
      lappend global_setupdata(em_ved) $ved
    } elseif {[ved_setup_$i eval info exists triggermaster] && 
        [ved_setup_$i eval set triggermaster]} {
      lappend global_setupdata(ccm_ved) $ved
    } elseif {[ved_setup_$i eval info exists noreadout] && 
        [ved_setup_$i eval set noreadout]} {
      # no readout; only setup
      lappend global_setupdata(other_ved) $ved
    } else {
      lappend global_setupdata(cc_ved) $ved
    }
  }
  if {$global_verbose} {output        "all_veds : $global_setupdata(all_veds)"}
  if {$global_verbose} {output_append "em_ved   : $global_setupdata(em_ved)"}
  if {$global_verbose} {output_append "ccm_ved  : $global_setupdata(ccm_ved)"}
  if {$global_verbose} {output_append "cc_ved   : $global_setupdata(cc_ved)"}
  if {$global_verbose} {output_append "other_ved: $global_setupdata(other_ved)"}
  set l [llength $global_setupdata(em_ved)] 
  if {$l!=1} {
    output "WARNING! There are $l Eventbuilders defined." tag_under
  }
  set l [llength $global_setupdata(ccm_ved)] 
  if {$l!=1} {
    output "WARNING! There are $l Mastertriggers defined." tag_under
  }
  set l [llength $global_setupdata(all_veds)] 
  if {$l==0} {
    output "WARNING! There are no VEDs defined." tag_under
  }
  create_status_frames
  return 0
}

proc check_ved_status {requested} {
  global global_setupdata
  set res 0
  foreach i $global_setupdata(all_veds) {
    if [catch {set ro_stat [$i readout status]} mist] {
      output "VED [$i name]:\n$mist" tag_red
      set res -1
    } else {
#      output $ro_stat
      set stat [lindex $ro_stat 0]
      if {$stat!=$requested} {
        output "VED [$i name]: readoutstatus $stat" tag_red
        set res -1
      }
    }
  }
  return $res
}
