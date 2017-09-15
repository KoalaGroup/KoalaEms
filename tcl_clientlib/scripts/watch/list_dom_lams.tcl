# lists all lam domains

proc list_dom_lams {ved} {

if [catch {set lams [$ved namelist 2 2]} mist] {puts $mist; return}

if {$lams == ""} {
  puts "--- no lam domains defined"
} else {
  puts "--- lam domains:"
  foreach i $lams {list_dom_lam $ved $i}
}
}
