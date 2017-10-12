# $Id: unsol_actioncompleted.tcl,v 1.1 2000/08/31 15:43:10 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc do_actioncompleted {space v h d} {
  set do [lindex $d 1]
  set typ [lindex $d 2]
  set res [lindex $d 3]
  set text "VED [$v name] ([set vedsetup_${space}::description]), dataout $do: "
  switch $typ {
    1 {append text "Filemark written"}
    2 {append text "Wind complete"}
    3 {append text "Rewind complete"}
    4 {append text "Wind to end of data complete"}
    5 {append text "Unload complete"}
    6 {append text "unknown action $typ complete"}
    }
  if {$res==0} {
    output "$text" tag_green
  } else {
    output "$text; Error $res" tag_red
  }
}

proc unsol_actioncompleted {space v h d} {
  set compl_type [lindex $d 0]
  switch $compl_type {
    0 {output "ved $v: actioncompleted: completion_none" tag_orange}
    1 {do_actioncompleted $space $v $h $d}
    default {output "ved $v: actioncompleted: completion_type $compl_type" tag_orange}
  }
}
