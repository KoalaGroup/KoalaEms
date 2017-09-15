# lists the trigger domain (there is only one)

proc list_dom_trigger {ved} {

puts "domain trigger:"
if [catch {puts "  [$ved trigger upload]"} mist] {puts $mist}
}

proc changed_dom_trigger {ved data} {
list_dom_trigger $ved
}
