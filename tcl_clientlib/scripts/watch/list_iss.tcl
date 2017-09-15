# lists all instrumentation systems

proc list_iss {ved} {

if [catch {set iss [$ved namelist 3]} mist] {puts $mist; return}

if {$iss == ""} {
  puts "--- no instrumentation systems defined"
} else {
  puts "--- instrumentation systems:"
  foreach i $iss {list_is $ved $i}
}
}
