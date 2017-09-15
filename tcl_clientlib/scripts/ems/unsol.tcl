#
#
#
#
#
#
#

set unsol_library /usr/local/ems/lib/unsol
lappend auto_path $unsol_library

create_output "Unsol. Messages"

rename ems_open _ems_open

ems_unsolcommand 1 {output "server %v tot"; %v close}
ems_unsolcommand 2 {output "commu tot"; ems_disconnect}

proc ved_unsol {ved header data} {
output "ved [$ved name]:"
output_append "  size  = [lindex $header 0]"
output_append "  ved   = [lindex $header 2]"
output_append "  type  = [lindex $header 3]"
output_append "  flags = [lindex $header 4]"
set i 0
foreach d $data {
  output_append " $i: $d"
  incr i
}
}

proc ems_open {ved args} {
set x "_ems_open $ved"
if {[llength $args] == 0} {
    set x "$x controller"
  } else {
    foreach i $args {set x "$x $i"}
  }
set ved [eval $x]
$ved unsol {ved_unsol %v %h %d}
return $ved
}
