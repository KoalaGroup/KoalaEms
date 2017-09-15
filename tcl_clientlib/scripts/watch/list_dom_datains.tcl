# lists all datain domains

proc list_dom_datains {ved} {

if [catch {set dis [$ved namelist 2 5]} mist] {puts $mist; return}

if {$dis == ""} {
  puts "--- no datain domains defined"
} else {
  puts "--- datain domains:"
  foreach i $dis {list_dom_datain $ved $i}
}
puts ""
}
