# $Id: unsol.tcl,v 1.2 1998/09/14 12:10:15 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars in this file:
#
# global_after(outer_loop)
#

proc ems_unsol_bye {} {
  global global_after

  disconnect_commu
  .bar configure -background #f00
  foreach i [array names global_after] {
    after cancel $global_after($i)
  }
  set global_after(outer_loop) [after 60000 outer_loop]
}

proc ems_unsol_dead {} {
  global global_after

  foreach i [array names global_after] {
    after cancel $global_after($i)
  }
  set global_after(outer_loop) [after 100 outer_loop]
}
