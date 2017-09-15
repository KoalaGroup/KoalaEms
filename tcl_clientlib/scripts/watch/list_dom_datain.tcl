# lists a single datain domain

proc list_dom_datain {ved idx} {

puts "datain domain $idx:"
if [catch {puts "  [$ved datain upload $idx]"} mist] {puts $mist}
}

proc changed_dom_datain {ved data} {

set idx [lindex $data 0]
puts "datain domain $idx:"
if [catch {puts "  [$ved datain upload $idx]"} mist] {puts $mist}
}
