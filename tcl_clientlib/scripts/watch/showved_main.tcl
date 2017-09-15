# main procedure of showved
# uses global vars:
#   posarg  // name of VED
#   ved     // VED object

proc printunsol {ved header data} {
  puts "unsol. message from VED [$ved name]:"
  puts "header: $header"
  puts "data: $data"
}

proc newstatus {ved data} {
  set action [lindex $data 0]
  set objtype [lindex $data 1]
  set idx [lindex $data 2]
  set rest [lrange $data 2 end]

  switch $objtype {
    1 {set objname "ved"}
    2 {set objname "domain"}
    3 {set objname "is"}
    4 {set objname "variable"}
    5 {set objname "programinvocation"}
    6 {set objname "dataout"}
    default {set objname "unknown objecttype $objtype"}
  }
  switch $action {
    1 {set actname "new"}
    2 {set actname "deleted"}
    3 {set actname "changed"}
    4 {set actname "started"}
    5 {set actname "stopped"}
    6 {set actname "reset"}
    7 {set actname "resumed"}
    8 {set actname "enabled"}
    9 {set actname "disabled"}
    default {set actname "unknown action $action"}
  }
  puts "$actname: ${objname}($idx)"
  if {$action==1 || $action==3} {
    switch $objtype {
      1 {changed_ved $ved $rest}
      2 {changed_domain $ved $rest}
      3 {changed_is $ved $rest}
      4 {changed_variable $ved $rest}
      5 {changed_programinvocation $ved $rest}
      6 {changed_dataout $ved $rest}
    }
  } elseif {$action==2} {
    puts "object $objname [lindex $data 2] [lindex $data 3] deleted"
  }
}

proc server_dead {ved} {
  global waiter
  puts "VED [$ved name] dead"
  set waiter 1
}

proc good_bye {} {
  global waiter
  puts "commu requests disconnect"
  set waiter 1
}

proc bgerror {args} {
  puts "$args"
}

proc main {} {
global posarg ved namelist waiter

# parse commandline arguments
process_args

# VEDname must be given
if {[info exists posarg(0)]==0} {
  puts "usage showved (\[-h <host>\] \[-p <port>\])|\[-s <socket>\] <ved_name>"
  return -1
  }

if {[connect_commu]!=0} {return -1}

if {[open_ved $posarg(0)]!=0} {return -1}

# get list of implemented objects
if [catch {set namelist [$ved namelist null]} mist] {
  puts $mist
  ems_disconnect
  $ved close
  return -1
  }

puts "[llength $namelist] basic object types found:"
foreach obj $namelist {
  switch $obj {
    1 {puts "  VED"}
    2 {puts "  domain"}
    3 {puts "  instrumentation system"}
    4 {puts "  variable"}
    5 {puts "  program invocation"}
    6 {puts "  dataoutput"}
    default {puts "  unknown objecttype $obj"}
  }
}
puts ""

$ved unsol {printunsol %v %h %d}
$ved typedunsol StatusChanged {newstatus %v %d}
ems_unsolcommand ServerDisconnect {server_dead %v}
ems_unsolcommand Bye {good_bye}

list_objects $ved $namelist [info exists namedarg(v)]

# puts "now waiting for unsolicited messages"
# set waiter 0
# vwait waiter

$ved close
ems_disconnect

return 0
}
