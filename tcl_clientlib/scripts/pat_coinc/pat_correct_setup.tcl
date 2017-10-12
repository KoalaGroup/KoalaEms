# $ZEL: pat_correct_setup.tcl,v 1.1 2002/09/26 12:15:12 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur vervollstaendigt die Variablen, die von einem Konfigfile
# gelesen wurden. Als Ergebnis sind alle Setupvariablen vorhanden und
# initialisiert, deren Existenz spaeter vorausgesetzt wird
#

proc correct_setup {} {
  global global_setup
  #global global_setup_names
  #global global_setup_vis

  if {[info exists global_setup(commu_transport)] == 0} {
    set global_setup(commu_transport) default}

  if {[info exists global_setup(commu_host)] == 0} {
    set global_setup(commu_host) localhost}

  if {[info exists global_setup(commu_socket)] == 0} {
    set global_setup(commu_socket) /var/tmp/emscomm}

  if {[info exists global_setup(commu_port)] == 0} {
    set global_setup(commu_port) 4096}

  if {[info exists global_setup(show_name)] == 0} {
    set global_setup(show_name) 1}

  if {[info exists global_setup(show_rate)] == 0} {
    set global_setup(show_rate) 1}

  if {[info exists global_setup(termfont)] == 0} {
    set global_setup(termfont) -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*}

  if {[info exists global_setup(interval)] == 0} {
    set global_setup(interval) 10000}

  if {[info exists global_setup(ved)] == 0} {
    set global_setup(ved) t02}

  if {[info exists global_setup(var)] == 0} {
    set global_setup(var) 10}

  if {[info exists global_setup(no_wm)] == 0} {
    set global_setup(no_wm) 0}
}
