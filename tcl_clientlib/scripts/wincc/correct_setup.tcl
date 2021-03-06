# $Id: correct_setup.tcl,v 1.1 2000/07/15 21:34:37 wuestner Exp $
# ? 1999 P. W?stner; Zentrallabor f?r Elektronik; Forschungszentrum J?lich
#
# Diese Prozedur vervollstaendigt die Variablen, die von einem Konfigfile
# gelesen wurden. Als Ergebnis sind alle Setupvariablen vorhanden und
# initialisiert, deren Existenz spaeter vorausgesetzt wird
#

proc correct_setup {} {
  global global_setup

  if {![info exists global_setup(maintitle)]} {
    set global_setup(maintitle) "WINCC Interface"}

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

  if {![info exists global_setup(wincc_host)]} {
    set global_setup(wincc_host) zelz35}

  if {![info exists global_setup(wincc_port)]} {
    set global_setup(wincc_port) 4001}

  if {![info exists global_setup(ved_name)]} {
    set global_setup(ved_name) 064}

  if {![info exists global_setup(loglength)]} {
    set global_setup(loglength) 1000}

}
