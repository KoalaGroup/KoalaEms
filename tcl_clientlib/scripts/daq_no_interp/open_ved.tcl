# $Id: open_ved.tcl,v 1.13 2000/08/31 15:43:08 wuestner Exp $
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
# global_namespaces($ved)   ; namespacename fuer alle veds
#

proc open_veds {} {
  global global_setupdata global_verbose global_ved_descriptions global_veds
  global global_veds_running global_veds_writing global_veds_reading
  global global_namespaces global_dataoutlist

  if {![ems_connected]} {
    output "try connect to commu" tag_orange
    if [connect_commu] {return -1}
    output_append "...OK" tag_orange
  }
  set global_setupdata(all_veds) {}
  set global_setupdata(em_ved) {}
  set global_setupdata(ccm_ved) {}
  set global_setupdata(cc_ved) {}
  if [info exists global_veds] {unset global_veds}
  set openveds [ems_openveds]
  set num 0
  #output "names: $global_setupdata(names)"
  foreach i $global_setupdata(names) {
    #output "name: $i"
    set vedname [namespace eval vedsetup_$i {set vedname}]
    #output_append "vedname: $vedname"
    set global_ved_descriptions($vedname) [namespace eval vedsetup_$i {set description}]
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
      #output "set global_dataoutlist($ved) {}"
      set global_dataoutlist($ved) {}
    } elseif {$global_verbose} {
      if {!$num} {output "open VEDs:"}
      incr num
      output_append "  $vedname already open"
    }
    set global_namespaces($ved) vedsetup_$i
    set global_veds($vedname) $ved
    set vedsetup_${i}::ved $ved

    #output "add $ved to all_ved"
    lappend global_setupdata(all_veds) $ved
    #output_append "all_ved: $global_setupdata(all_veds)"
    set global_veds_running($ved) 0
    set global_veds_writing($ved) 0
    set global_veds_reading($ved) 0
    if {[info exists vedsetup_${i}::eventbuilder] &&
        [set vedsetup_${i}::eventbuilder]} {
      lappend global_setupdata(em_ved) $ved
    } elseif {[info exists vedsetup_${i}::triggermaster] && 
        [set vedsetup_${i}::triggermaster]} {
      lappend global_setupdata(ccm_ved) $ved
    } else {
      lappend global_setupdata(cc_ved) $ved
    }
  }
  if {$global_verbose} {output        "all_veds: $global_setupdata(all_veds)"}
  if {$global_verbose} {output_append "em_ved  : $global_setupdata(em_ved)"}
  if {$global_verbose} {output_append "ccm_ved : $global_setupdata(ccm_ved)"}
  if {$global_verbose} {output_append "cc_ved  : $global_setupdata(cc_ved)"}
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
