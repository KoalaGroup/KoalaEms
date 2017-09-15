# $Id: correct_setup.tcl,v 1.1 2000/07/15 21:37:00 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# Diese Prozedur vervollstaendigt die Variablen, die von einem Konfigfile
# gelesen wurden. Als Ergebnis sind alle Setupvariablen vorhanden und
# initialisiert, deren Existenz spaeter vorausgesetzt wird
#

proc correct_setup {} {
  global global_setup

  if {![info exists global_setup(maintitle)]} {
    set global_setup(maintitle) "ZEL Deadtime Display"}

  if {![info exists global_setup(auto_save)]} {
    set global_setup(auto_save) 0}

  if {![info exists global_setup(commu_transport)]} {
    set global_setup(commu_transport) default}

  if {![info exists global_setup(commu_host)]} {
    set global_setup(commu_host) localhost}

  if {![info exists global_setup(commu_socket)]} {
    set global_setup(commu_socket) /var/tmp/emscomm}

  if {![info exists global_setup(commu_port)]} {
    set global_setup(commu_port) 4096}

  if {![info exists global_setup(loglength)]} {
    set global_setup(loglength) 1000}

  if {![info exists global_setup(loop_delay)]} {
    set global_setup(loop_delay) {10000}}
}
