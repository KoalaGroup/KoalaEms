# $Id: connect_loop.tcl,v 1.1 1998/09/29 13:41:24 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

proc try_connect {} {
  global ems_nocount
  global ems_noshow

  set ems_nocount 1
  set ems_noshow 1
  ems_unsolcommand 2 {ems_unsol_bye}
  connect_commu
  if [ems_connected] {return 0} else {return -1}
}
