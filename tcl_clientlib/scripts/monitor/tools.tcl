# $Id: tools.tcl,v 1.2 1998/10/07 10:37:15 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# errorCode
# errorInfo
#

proc bgerror {args} {
  global errorCode errorInfo
  putm "background error:\n$errorCode\n$errorInfo"
}

proc namespacedummies {} {
#  dummy_supersetupfile
#  dummy_run_nr
}
