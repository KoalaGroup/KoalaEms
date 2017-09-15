# lists the ved object

proc list_ved {ved verbose} {

puts "=== object VED:"
if [catch {$ved identify} mist] {
  puts $mist
} else {
  set lved $mist
  puts "  ved version          : [lindex $lved 0]"
  puts "  version of requesttab: [lindex $lved 1]"
  puts "  version of unsoltab  : [lindex $lved 2]"
  puts "  name of VED: [lindex $lved 3]"
  puts "  date of compilation: [lindex $lved 4]"
  if {$verbose} {
    puts "  configuration:"
    puts [lindex $lved 5]
    }
}
puts ""
}
