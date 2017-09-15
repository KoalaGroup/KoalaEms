# $ZEL: timestamp.tcl,v 1.6 2007/04/15 13:15:09 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_lasttime
#

proc timestamp {immer} {
  global global_lasttime clockformat

  set seconds [clock seconds]
  if {$immer || ([expr $global_lasttime+60]<$seconds)} {
    output "  -- [clock format $seconds -format $clockformat] --"
    set global_lasttime $seconds
  }
}
