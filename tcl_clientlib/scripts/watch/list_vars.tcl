# lists all existing variables

proc list_vars {ved} {

if [catch {set vars [$ved namelist 4]} mist] {puts $mist; return}

if {$vars == ""} {
  puts "--- no variables defined"
} else {
  puts "--- variables:"
  foreach i $vars {list_var $ved $i}
}
}
