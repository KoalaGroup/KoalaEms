# $Id: unsol.tcl,v 1.1 1998/09/29 13:41:30 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars in this file:
#
# global_running(outer_loop)
# global_after(outer_loop)
#

proc ems_unsol_bye {} {
  global global_after global_running

  disconnect_commu
  putm "disconnected"
  .bar configure -background #f00
  if {$global_running(outer_loop)} {
    foreach i [array names global_after] {
      after cancel $global_after($i)
    }
  set global_after(outer_loop) [after 60000 outer_loop]
  }
}
