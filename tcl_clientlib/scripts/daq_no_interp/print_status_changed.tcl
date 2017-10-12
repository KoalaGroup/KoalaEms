# $Id: print_status_changed.tcl,v 1.2 1999/08/07 20:35:48 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
proc print_unsol_statuschanged {v d} {

  output "unsol_statuschanged from VED $v:"
  output_append $d
  set action [lindex $d 0]
  set object [lindex $d 1]
  set text "object "
  switch $object {
    0 {append text "null "}
    1 {append text "ved "}
    2 {
      append text "domain "
      set domain [lindex $d 2]
      switch $domain {
        0 {append text "null "}
        1 {append text "Modullist "}
        2 {append text "LAMproclist [lindex $d 3] "}
        3 {append text "Trigger "}
        4 {append text "Event "}
        5 {append text "Datain [lindex $d 3] "}
        6 {append text "Dataout [lindex $d 3] "}
        default {append text "$domain "}
      }
    }
    3 {append text "is [lindex $d 2] "}
    4 {append text "var [lindex $d 2] "}
    5 {
      append text "pi "
      set pi [lindex $d 2]
      switch $pi {
        0 {append text "null "}
        1 {append text "readout "}
        2 {append text "LAM [lindex $d 3] "}
        default {append text "$pi "}
      }
    }
    6 {append text "do [lindex $d 2] "}
    default {append text "$object "}
  }
  switch $action {
    0  {append text "unchanged"}
    1  {append text "created"}
    2  {append text "deleted"}
    3  {append text "changed"}
    4  {append text "started"}
    5  {append text "stopped"}
    6  {append text "reset"}
    7  {append text "resumed"}
    8  {append text "enabled"}
    9  {append text "disabled"}
    10 {append text "finished"}
    default {append text ", unknown action $action"}
  }
  output_append $text
  
}
