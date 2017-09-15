# $ZEL: tools.tcl,v 1.11 2003/02/04 19:28:01 wuestner Exp $
# copyright: 1998, 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# errorCode
# errorInfo
#

proc bgerror {args} {
  global errorCode errorInfo
  output "background error:" tag_blue
  if {$errorCode!="NONE"} {output_append "errorCode: $errorCode" tag_blue}
  output_append $errorInfo tag_blue
}

proc daq_ende {} {
  close_log
  ende
}

proc namespacedummies {} {
  dummy_run_nr
  dummy_source_new
  dummy_headers
}

proc wait_handler {} {
  global global_waitvar
  set global_waitvar 1
}

proc stop_wait_actions {} {
  global global_waitvar global_waitproc
  if [info exists global_waitproc] {
    after cancel $global_waitproc
  }
  set global_waitvar -1
}
