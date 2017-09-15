# $ZEL: pat_useargs.tcl,v 1.2 2003/09/10 21:38:33 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
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

  if [info exists namedarg(w)] {
    set global_setup(no_wm) 1
  }
}
