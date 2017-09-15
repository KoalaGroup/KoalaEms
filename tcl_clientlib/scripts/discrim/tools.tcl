# $Id: tools.tcl,v 1.1 2000/03/24 17:13:23 wuestner Exp $
#

proc bgerror {args} {
  global errorCode errorInfo
  puts "background error:"
  if {$errorCode!="NONE"} {puts "errorCode: $errorCode"}
  puts $errorInfo
}

proc ende_ {} {
  #close_log
  #ende
  exit
}

proc namespacedummies {} {
  #dummy_supersetupfile
  #dummy_run_nr
}
