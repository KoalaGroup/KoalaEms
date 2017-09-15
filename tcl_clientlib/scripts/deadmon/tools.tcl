# $Id: tools.tcl,v 1.1 2000/07/15 21:37:02 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# errorCode
# errorInfo
#

proc bgerror {args} {
  global errorCode errorInfo
  LOG::put "background error:" tag_blue
  if {$errorCode!="NONE"} {LOG::append "errorCode: $errorCode" tag_blue}
  LOG::append $errorInfo tag_blue
}

proc prog_ende {} {
  ende
}

proc namespacedummies {} {
  dummy_VED
  dummy_LOOP
}
