# lists a single dataout domain

proc list_dom_dataout {ved idx} {

puts "domain dataout $idx:"
if [catch {puts "  [$ved dataout upload $idx]"} mist] {puts $mist}
}

proc changed_dom_dataout {ved data} {

set idx [lindex $data 0]
puts "domain dataout $idx:"
if [catch {puts "  [$ved dataout upload $idx]"} mist] {puts $mist}
}
