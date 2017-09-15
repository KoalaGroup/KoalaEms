# connect communication process
# uses global vars:
#   namedarg(s) // unix domain socket
#   namedarg(h) // hostname
#   namedarg(p) // port number

proc connect_commu {} {
global namedarg

if [catch {
  if [info exists namedarg(s)] {
    puts "execute: ems_connect $namedarg(s)"
    ems_connect $namedarg(s)
    } elseif {[info exists namedarg(h)] || [info exists namedarg(p)]} {
      set host "localhost"
      set port 4096
      if [info exists namedarg(h)] {set host $namedarg(h)}
      if [info exists namedarg(p)] {set port $namedarg(p)}
      puts "execute: ems_connect $host $port"
      ems_connect $host $port
    } else {
      puts "execute: ems_connect"
      ems_connect
    }
  } mist] {
  puts $mist
  return -1
  }
return 0
}
