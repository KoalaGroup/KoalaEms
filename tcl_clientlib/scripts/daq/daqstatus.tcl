# $ZEL: daqstatus.tcl,v 1.6 2008/11/11 18:13:45 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_daq(statustext)
# global_daq(autoreset): 0/1
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
  if {$status!="?"} {
    set global_daq(status) $status
    if {$status=="running"} {
      set global_daq(autoreset) 0
      set global_daq(auto_restart_message) 0
    }
  }
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
