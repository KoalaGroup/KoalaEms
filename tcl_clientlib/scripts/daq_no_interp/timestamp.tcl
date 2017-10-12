# $Id: timestamp.tcl,v 1.4 1999/04/09 12:32:46 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_lasttime
#

proc timestamp {immer} {
  global global_lasttime
  set seconds [clock seconds]
  if {$immer || ([expr $global_lasttime+60]<$seconds)} {
    output "  -- [clock format $seconds] --"
    set global_lasttime $seconds
  }
}
