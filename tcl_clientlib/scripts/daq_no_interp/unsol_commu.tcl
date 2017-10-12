# $Id: unsol_commu.tcl,v 1.5 2000/08/31 15:43:10 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc server_disconnected {v} {
  global global_veds
  output "server [$v name] disconnected (probably dead)." tag_red
  unset global_veds([$v name])
}

proc commu_bye {} {
  output "commu says bye. :=\{" tag_red
  ems_disconnect
  output "The system is now in an unusable state." tag_red
  output_append "You may have to start a new commu and reinitialize all servers." tag_red
}
