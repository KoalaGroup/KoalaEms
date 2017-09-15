#
# Diese Prozedur vervollstaendigt die Variablen, die von einem Konfigfile
# gelesen wurden. Als Ergebnis sind alle Setupvariablen vorhanden und
# initialisiert, deren Existenz spaeter vorausgesetzt wird
#

proc correct_setup {} {
  global global_setup

  if {[info exists global_setup(commu_transport)] == 0} {
    set global_setup(commu_transport) default}

  if {[info exists global_setup(commu_host)] == 0} {
    set global_setup(commu_host) localhost}

  if {[info exists global_setup(commu_socket)] == 0} {
    set global_setup(commu_socket) /var/tmp/emscomm}

  if {[info exists global_setup(commu_port)] == 0} {
    set global_setup(commu_port) 4096}

  if {[info exists global_setup(veds)] == 0} {
    set global_setup(veds) {}}

  if {[info exists global_setup(termfont)] == 0} {
    set global_setup(termfont) -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*}

  if {[info exists global_setup(interval)] == 0} {
    set global_setup(interval) 10000}
}
