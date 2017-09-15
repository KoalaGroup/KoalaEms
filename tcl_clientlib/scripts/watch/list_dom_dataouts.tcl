# lists all dataout domains

proc list_dom_dataouts {ved} {

if [catch {set dos [$ved namelist 2 6]} mist] {puts $mist; return}

if {$dos == ""} {
  puts "--- no dataout domains defined"
} else {
  puts "--- dataout domains:"
  foreach i $dos {list_dom_dataout $ved $i}
}
puts ""
}
