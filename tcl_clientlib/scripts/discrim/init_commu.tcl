# $Id: init_commu.tcl,v 1.2 2005/04/06 19:28:47 wuestner Exp $

proc init_commu_and_ved {} {
  global global_setup global_ved

  if [catch {
    switch $global_setup(commu_transport) {
      default {ems_connect}
      unix    {ems_connect $global_setup(commu_socket)}
      tcp     {ems_connect $global_setup(commu_host) $global_setup(commu_port)}
    }} mist] {
    puts "connect to commu: $mist"
    return -1
  } else {
    puts "connected to commu via [ems_connection]"
  }
  if [catch {set global_ved [ems_open $global_setup(ved)]} mist] {
    puts "open ved $global_setup(ved): $mist"
    return -1
  }

return 0
}
