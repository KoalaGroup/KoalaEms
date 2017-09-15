# lists all existing dataout objects of the given VED

proc list_dos {ved} {

if [catch {set dos [$ved namelist 6]} mist] {puts $mist; return}

if {$dos == ""} {
  puts "--- no dataout objects defined"
} else {
  puts "--- dataout objects:"
  foreach i $dos {list_do $ved $i}
}
puts ""
}
