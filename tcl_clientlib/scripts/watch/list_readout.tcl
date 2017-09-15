#lists the readout (there is only one)

proc list_readout {ved} {

puts "readout:"
if [catch {puts "  [$ved readout status]"} mist] {puts $mist}
puts ""
}
