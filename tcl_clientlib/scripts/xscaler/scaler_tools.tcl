# $ZEL: scaler_tools.tcl,v 1.2 2006/08/13 17:32:09 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# errorCode
# errorInfo
#

proc bgerror {args} {
  global errorCode errorInfo
  output "background error:"
  if {$errorCode!="NONE"} {output_append "errorCode: $errorCode"}
  output_append $errorInfo
}

proc namespacedummies {} {
#  dummy_supersetupfile
#  dummy_run_nr
}
