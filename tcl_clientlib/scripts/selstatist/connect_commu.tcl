# $Id: connect_commu.tcl,v 1.1 1998/09/29 13:41:23 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# connect_commu versucht, die Verbindung zum Kommunikationsprozess aufzunehmen
#

proc connect_commu {} {
  global global_setup

  putm "connect to commu"
  update idletasks
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
    putm "connected"
    set res 0
  }
  return $res
}

proc disconnect_commu {} {
  if [ems_connected] {
    putm "disconnect from commu"
    if [catch ems_disconnect mist] {
      putm $mist
    } else {
      putm "disconnected"
    }
  }
}
