# $ZEL: pat_tools.tcl,v 1.1 2002/09/26 12:15:13 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# errorCode
# errorInfo
#

proc bgerror {args} {
  global errorCode errorInfo
  puts "background error:"
  if {$errorCode!="NONE"} {puts "errorCode: $errorCode"}
  puts $errorInfo
}

proc namespacedummies {} {
#  dummy_supersetupfile
#  dummy_run_nr
}
