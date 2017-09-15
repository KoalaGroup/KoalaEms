#
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
# $Id: vme_connect.tcl,v 1.1 2000/07/15 21:33:35 wuestner Exp $
#

proc init_connection {} {
  global global_ved

  set global_ved ""
}

proc connect_to_server {} {
  global global_setup global_ved global_verbose

  disconnect_from_server

  if [catch {
    switch $global_setup(commu_transport) {
      default {ems_connect}
      unix    {ems_connect $global_setup(commu_socket)}
      tcp     {ems_connect $global_setup(commu_host) $global_setup(commu_port)}
    }} mist] {
    output "connect to commu: $mist" tag_red
    return -1
  } else {
    output "connected to commu via [ems_connection]"
    ems_unsolcommand ServerDisconnect {server_disconnected %v}
    ems_unsolcommand Bye commu_bye
  }
  if [catch {
    set global_ved [ems_open $global_setup(ved_name)]
  } mist] {
    output "open ved: $mist" tag_red
    set global_ved ""
    return -1
  }
  output "connected to server" tag_green
  return 0
}

proc disconnect_from_server {} {
  global global_setup global_ved global_verbose

  if [ems_connected] {
    if {$global_ved!=""} {
      if [catch {
        $global_ved close
      } mist] {
        output "close ved: $mist" tag_orange
      }
    }
    if [catch {
      ems_disconnect
    } mist] {
      output "disconnect from commu: $mist" tag_orange
    }
  }
  set global_ved ""
}
