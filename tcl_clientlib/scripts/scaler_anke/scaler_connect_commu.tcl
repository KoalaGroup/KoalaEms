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
    set res -1
  } else {
    output_append "ok."
    set res 0
  }
  return $res
}

proc disconnect_commu {} {
  global global_setup

  output "disconnect from commu ..."
  ems_disconnect
  output_append "ok."
}
