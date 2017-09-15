#
# wincc/wincc_connect
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
# $Id: wincc_connect.tcl,v 1.1 2000/07/15 21:34:40 wuestner Exp $
#

proc init_connection {} {
  global global_wcc

  set global_wcc(ved) ""
  set global_wcc(path) -1
}

proc connect_to_server {} {
  global global_setup global_wcc global_verbose

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
    set global_wcc(ved) [ems_open $global_setup(ved_name)]
  } mist] {
    output "open ved: $mist" tag_red
    set global_wcc(ved) ""
    return -1
  }
  if [catch {
   set global_wcc(path) [$global_wcc(ved) command1 WCC_open '$global_setup(wincc_host)' $global_setup(wincc_port)]
  } mist] {
    output "WCC_open: $mist" tag_red
    set global_wcc(path) -1
    return -1
  }
  output "connected to server" tag_green
  return 0
}

proc disconnect_from_server {} {
  global global_setup global_wcc global_verbose

  if [ems_connected] {
    if {$global_wcc(ved)!=""} {
      if {$global_wcc(path)!=-1} {
        if [catch {
          $global_wcc(ved) command1 WCC_close $global_wcc(path)
        } mist] {
          output "WCC_close: $mist" tag_orange
        } 
      }
      if [catch {
        $global_wcc(ved) close
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
  set global_wcc(ved) ""
  set global_wcc(path) -1
}
