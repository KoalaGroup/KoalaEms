# $Id: resume.tcl,v 1.4 1999/04/09 12:32:39 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_verbose
# global_daq(status)
# global_setupdata(em_ved)  ; Eventbuilder
# global_setupdata(ccm_ved) ; Cratecontroller mit Mastertrigger
# global_setupdata(cc_ved)  ; Cratecontroller
#

proc info_not_pause_status {} {
  set win .daq_question
  set text {DAQ is not in pause status.}
  tk_dialog $win "DAQ Message" $text warning 0 {OK}
}

proc resume_button {} {
  global global_daq global_setupdata global_verbose

  if {$global_daq(status)!="pause"} {
    info_not_pause_status
    return
  }

  output "RESUME" time tag_under
  if {[llength $global_setupdata(ccm_ved)]>0} {
# restart only triggermaster
    set veds $global_setupdata(ccm_ved)
  } elseif {[llength $global_setupdata(cc_ved)]>0} {
# restart all crate controllers
    set veds $global_setupdata(cc_ved)
  } else {
# restart eventbuilder
    set veds $global_setupdata(em_ved)
  }

  foreach i $veds {
    if {$global_verbose} {output "restart $i"}
    if [catch {$i readout resume} mist] {
      output "resume $i: $mist"
    }
  }
  change_status running "Running"
}
