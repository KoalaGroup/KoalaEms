# $Id: connect_commu.tcl,v 1.3 1998/09/17 12:16:06 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# connect_commu versucht, die Verbindung zum Kommunikationsprozess aufzunehmen
#

proc connect_commu {} {
  global global_setup
  global ems_nocount
  global ems_noshow

  putm "connect to commu"
  update idletasks
  set ems_nocount 1
  set ems_noshow 1
  ems_unsolcommand 2 {ems_unsol_bye}
  ems_unsolcommand 1 {ems_unsol_dead}
  if [catch {
    switch -exact $global_setup(commu_transport) {
      default {ems_connect}
      unix    {
        ems_connect $global_setup(commu_socket)
        }
      tcp     {
        ems_connect $global_setup(commu_host) $global_setup(commu_port)
        }
    }} mist] {
    putm $mist
    disconnect_commu
    set res -1
  } else {
    set res 0
  }
  return $res
}

proc disconnect_commu {} {
  if [ems_connected] {
    if [catch ems_disconnect mist] {
      putm $mist
    }
  }
}
