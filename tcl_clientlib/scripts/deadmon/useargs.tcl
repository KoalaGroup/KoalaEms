# $Id: useargs.tcl,v 1.1 2000/07/15 21:37:02 wuestner Exp $
#

proc use_args {} {
  global global_setup namedarg

  if [info exists namedarg(s)] {
    set global_setup(commu_socket) $namedarg(s)
    set global_setup(commu_transport) unix
  }

  if [info exists namedarg(h)] {
    set global_setup(commu_host) $namedarg(h)
    set global_setup(commu_transport) tcp
  }

  if [info exists namedarg(p)] {
    set global_setup(commu_port) $namedarg(p)
    set global_setup(commu_transport) tcp
  }
}
