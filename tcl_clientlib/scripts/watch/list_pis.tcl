#list all program invocations

proc list_pis {ved} {

if [catch {set pis [$ved namelist 5 0]} mist] {puts $mist; return}
if {$pis == ""} {
  puts "--- no program invocations available"
} else {
  puts "--- program invocations (readout and lam):"
  foreach i $pis {
    switch $i {
      1 {list_readout $ved}
      2 {list_lams $ved}
      default {puts "unknown pitype $i"}
    }
  }
}
}
