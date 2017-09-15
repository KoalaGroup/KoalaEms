# list a single dataout object

proc list_do {ved idx} {
puts "object dataout $idx:"
if [catch {$ved dataout status $idx} mist] {
  puts $mist
} else {
  set st $mist
  puts "  [lindex $st 0]"
  puts "  [lindex $st 1]"
  puts "  [lindex $st 2]"
  puts "  optional values: [lrange $st 3 end]"
}
}
