# $Id: unsol_commu.tcl,v 1.1 2000/07/15 21:33:34 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# no global vars
#

proc server_disconnected {v} {
  output "server $v disconnected (probably dead)." tag_red
}

proc commu_bye {} {
  output "commu says bye. :=\{" tag_red
  ems_disconnect
  output "The system is now in an unusable state." tag_red
  output_append "You may have to start a new commu and reinitialize all servers." tag_red
}
