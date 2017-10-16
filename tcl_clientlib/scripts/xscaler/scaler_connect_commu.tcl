# $ZEL: scaler_connect_commu.tcl,v 1.3 2006/08/13 17:32:04 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# connect_commu versucht, die Verbindung zum Kommunikationsprozess aufzunehmen
#

proc connect_commu {} {
  global global_setup

  output "connect to commu ..."
  update idletasks
  if [catch {
    switch -exact $global_setup(commu_transport) {
      default {ems_connect}
      unix    {
        output_append "  use $global_setup(commu_socket)"
        ems_connect $global_setup(commu_socket)
        }
      tcp     {
        output_append "  use $global_setup(commu_port)@$global_setup(commu_host)"
        ems_connect $global_setup(commu_host) $global_setup(commu_port)
        }
    }} mist] {
    output $mist
    disconnect_commu
    set res -1
  } else {
    output_append "ok."
    set res 0
  }
  return $res
}

proc disconnect_commu {} {
  if [ems_connected] {
    output "disconnect from commu ..."
    if [catch ems_disconnect mist] {
      output_append $mist
    } else {
      output_append "ok."
    }
  }
}
