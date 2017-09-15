# lists the event domain (there is only one)

proc list_dom_event {ved} {

puts "last event:"
if [catch {puts "  [$ved event]"} mist] {puts $mist}
puts ""
}
