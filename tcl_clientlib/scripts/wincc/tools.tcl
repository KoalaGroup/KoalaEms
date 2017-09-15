# $Id: tools.tcl,v 1.1 2000/07/15 21:34:39 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
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

proc dummies {} {
  dummy_ved_setup
  dummy_wincc_setup
  dummy_varframe
  dummy_var_insert
}
