#
# global vars in this file:
#
# enum    global_setup(commu_transport) / {default, unix, tcp}
# string  global_setup(commu_host)
# string  global_setup(commu_socket)
# int     global_setup(commu_port)
# boolean global_setup(offline)
#

proc setup_commu {} {
  global commu

  set confmode [ems_confmode synchron]
  if [catch {set commu [ems_open commu_l]} mist] {
    output "open commu_l: $mist"
  } else {
    if [catch {$commu command {globalunsol {1}}} mist] {
      output "command globalunsol 1: $mist"
      output_append "Your commu seems to be too old."
    }
  }
  ems_confmode $confmode
}

proc connect_commu {printerror} {
  global global_setup global_commuerror_x

  if [catch {
    switch $global_setup(commu_transport) {
      default {ems_connect}
      unix    {ems_connect $global_setup(commu_socket)}
      tcp     {ems_connect $global_setup(commu_host) $global_setup(commu_port)}
    }} mist] {
    set res -1
    if {$printerror} {
      output "connect to commu: $mist"
    } else {
      set global_commuerror_x $mist
    }
  } else {
    setup_commu
    set res 0
  }
  return $res
}
