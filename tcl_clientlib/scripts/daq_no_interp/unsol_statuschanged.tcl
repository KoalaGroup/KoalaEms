# $Id: unsol_statuschanged.tcl,v 1.1 2000/08/31 15:43:12 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc unsol_statuschanged {space v h d} {
  set action [lindex $d 0]
  set object [lindex $d 1]

# objects:
#  0 null
#  1 VED
#  2 domain
#    0 null
#    1 Modullist
#    2 LAMproclist
#    3 Trigger
#    4 Event
#    5 Datain,
#    6 Dataout
#  3 IS
#  4 variable
#  5 PI
#    0 null
#    1 readout
#    2 LAM
#  6 dataout

  #print_unsol_statuschanged $v $d
  switch $object {
    1 {obj_ved_changed $v $action}
    2 {if {[lindex $d 2]==6} {dom_do_changed $v [lindex $d 3] $action}}
    5 {if {[lindex $d 2]==1} {obj_ro_changed $v [lindex $d 3] $action}}
    6 {obj_do_changed $v [lindex $d 2] $action}
    3 {}
    default {output_append "  unknown object $object" tag_orange}
  }
}
