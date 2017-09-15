# $ZEL: pause.tcl,v 1.5 2003/02/04 19:27:55 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_verbose
# global_daq(status)
# global_setupdata(em_ved)  ; Eventbuilder
# global_setupdata(ccm_ved) ; Cratecontroller mit Mastertrigger
# global_setupdata(cc_ved)  ; Cratecontroller
#

proc info_not_running_status {} {
  set win .daq_question
  set text {DAQ is not running.}
  tk_dialog $win "DAQ Message" $text warning 0 {OK}
}

proc pause_button {} {
  global global_daq global_setupdata global_verbose

  if {$global_daq(status)!="running"} {
    info_not_running_status
    return
  }

  output "PAUSE" time tag_under
  output "ccm_ved: $global_setupdata(ccm_ved)" tag_orange
  output "cc_ved: $global_setupdata(cc_ved)" tag_orange
  output "em_ved: $global_setupdata(em_ved)" tag_orange
  if {[llength $global_setupdata(ccm_ved)]>0} {
# stop only triggermaster
    set veds $global_setupdata(ccm_ved)
  } elseif {[llength $global_setupdata(cc_ved)]>0} {
# stop all crate controllers
    set veds $global_setupdata(cc_ved)
  } else {
# stop eventbuilder
    set veds $global_setupdata(em_ved)
  }
  foreach i $veds {
    if {$global_verbose} {output "pause $i"}
    if [catch {$i readout stop} mist] {
      output "pause $i: $mist"
    }
  }
  change_status pause "Pause"
}
