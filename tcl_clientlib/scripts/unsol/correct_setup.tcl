#
# Diese Prozedur vervollstaendigt die Variablen, die von einem Konfigfile
# gelesen wurden. Als Ergebnis sind alle Setupvariablen vorhanden und
# initialisiert, deren Existenz spaeter vorausgesetzt wird
#

proc correct_setup {} {
  global global_setup

  if {![info exists global_setup(auto_save)]} {
    set global_setup(auto_save) 0}

  if {![info exists global_setup(auto_start)]} {
    set global_setup(auto_start) 0}

  if {![info exists global_setup(commu_transport)]} {
    set global_setup(commu_transport) default}

  if {![info exists global_setup(commu_host)]} {
    set global_setup(commu_host) localhost}

  if {![info exists global_setup(commu_socket)]} {
    set global_setup(commu_socket) /var/tmp/emscomm}

  if {![info exists global_setup(commu_port)]} {
    set global_setup(commu_port) 4096}

  if {![info exists global_setup(veds)]} {
    set global_setup(veds) {}}

  if {[info exists global_setup(retryinterval)] == 0} {
    set global_setup(retryinterval) 60}

  if {[info exists global_setup(logfilename)] == 0} {
    set global_setup(logfilename) "ems_unsol.log"}

  if {![info exists global_setup(auto_log)]} {
    set global_setup(auto_log) 0}

  if {[info exists global_setup(loglength)] == 0} {
    set global_setup(loglength) 1000}
}
