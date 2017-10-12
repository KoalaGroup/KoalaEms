# $Id: strerror.tcl,v 1.1 2000/08/31 15:43:08 wuestner Exp $
# copyright 2000
#   P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc strerror {space errno} {
  set vedsetup_${space}::errno $errno
  namespace eval vedsetup_$space {
    if {![info exists uname]} {
      catch {set uname [$ved command1 Uname | stringlist]}
    }
    if {![info exists errno_arr($errno)]} {
      catch {set errno_arr($errno) [$ved command1 Strerror $errno | string]}
    }
    if [info exists errno_arr($errno)] {
      if [info exists uname] {
        set res "([lindex $uname 0]) "
      } else {
        set res ""
      }
      append res $errno_arr($errno)
    } else {
      set res "(bsd assumed) "
      append res [strerror_bsd $errno]
    }
    return $res
  }
}
