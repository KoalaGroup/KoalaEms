# lists a single instrumentation system

proc list_is {ved idx} {

puts "instrumentation system $idx:"
if [catch {set is [$ved is open $idx]} mist] {
  puts $mist
  return
}

if [catch {puts "  id     : [$is id]"} mist] {puts $mist}
if [catch {puts "  members: [$is memberlist upload]"} mist] {puts $mist}
if [catch {set status [$is status]} mist] {puts $mist; return}
puts "  status : $status"
set rol [lindex $status 3]
if {$rol == ""} {
  puts "  no readout lists defined"
} else {
  puts "  readout lists:"
  foreach i $rol {
    if [catch {puts "  list $i: [$is readoutlist upload $i]"} mist] {
      puts $mist
    }
  }
}
$is close
}

proc changed_is {ved data} {
set idx [lindex $data 0]
list_is $ved $idx
}
