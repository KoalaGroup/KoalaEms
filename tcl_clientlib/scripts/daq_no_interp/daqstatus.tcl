# $Id: daqstatus.tcl,v 1.4 1999/04/09 12:32:32 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_daq(statustext)
#

#
# global_daq(status):
#                     ______________________
#                    /                      \
#                    V                       |
# uninitialized --> idle <--> running <--> pause
#                       
# error == initialized
#

proc change_status {status text} {
  global global_daq
  if {[lsearch -exact {uninitialized idle running pause ?} $status]<0} {
    output "change_status called with wrong status code" tag_red
    output_append "text=$text"
  }
  if {$status!="?"} {set global_daq(status) $status}
  set global_daq(statustext) $text
  if [info exists global_daq(statusentry)] {
    $global_daq(statusentry) configure -foreground black -background $global_daq(statusbackcolor)
  }
}

proc change_status_fatal {} {
  global global_daq
  set global_daq(statustext) "FATAL ERROR"
  $global_daq(statusentry) configure -background black -foreground red
}
