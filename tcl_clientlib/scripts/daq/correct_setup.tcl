# $ZEL: correct_setup.tcl,v 1.10 2009/08/19 16:58:09 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur vervollstaendigt die Variablen, die von einem Konfigfile
# gelesen wurden. Als Ergebnis sind alle Setupvariablen vorhanden und
# initialisiert, deren Existenz spaeter vorausgesetzt wird
#

proc correct_setup {} {
  global global_setup

  if {![info exists global_setup(maintitle)]} {
    set global_setup(maintitle) "KOALA DAQ Control"}

  if {![info exists global_setup(facility)]} {
    set global_setup(facility) "KOALA@COSY"}

  if {![info exists global_setup(location)]} {
    set global_setup(location) "IKP, Research Center Juelich"}

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

  if {![info exists global_setup(logfiledir)]} {
    set global_setup(logfiledir) ~/logfiles}

  if {![info exists global_setup(loglength)]} {
    set global_setup(loglength) 1000}

  if {![info exists global_setup(maxlogfiles)]} {
    set global_setup(maxlogfiles) 100}

  if {![info exists global_setup(super_setup)]} {
    set global_setup(super_setup) ~/workspace/bt2019/preparation/fwd_installation/master.wad}

  if {![info exists global_setup(run_nr_file)]} {
    set global_setup(run_nr_file) {~/.run_nr}}

  if {![info exists global_setup(share_dir)]} {
    set global_setup(share_dir) {/usr/local/ems/share}}

  if {![info exists global_setup(tape_status_loop_delay)]} {
    set global_setup(tape_status_loop_delay) {10000}}

  if {![info exists global_setup(clockformat)]} {
    set global_setup(clockformat) {%Y-%m-%d %H:%M:%S %Z}}

  if {![info exists global_setup(run_commands_async)]} {
    set global_setup(run_commands_async) 1}
}
