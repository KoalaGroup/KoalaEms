# $ZEL: scaler_connect_loop.tcl,v 1.3 2006/08/13 17:32:04 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

proc try_connect {} {
  global ems_nocount

  set ems_nocount 1
  connect_commu
  if [ems_connected] {
    ems_unsolcommand 2 {ems_unsol_bye}
    return 0
  } else {
    return -1
  }
}
