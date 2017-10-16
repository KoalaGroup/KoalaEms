# $ZEL: scaler_outer_loop.tcl,v 1.2 2006/08/13 17:32:08 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_running(outer_loop)
# global_after(outer_loop)
#

proc outer_loop {} {
  global global_after
  
  if {[check_commands]!=0} {
    stop_outer_loop
    return
  }

  if {[try_connect]!=0} {
    output "connect without success; will retry after 60 seconds"
    set global_after(outer_loop) [after 60000 outer_loop]
    return
  }
  if {[open_veds]!=0} {
    disconnect_commu
    output "open without success; will retry after 60 seconds"
    set global_after(outer_loop) [after 60000 outer_loop]
    return
  }
  if {[open_is]!=0} {
    disconnect_commu
    output "open IS without success; will retry after 60 seconds"
    set global_after(outer_loop) [after 60000 outer_loop]
    return
  }
  foreach ved [ems_openveds] {
    $ved confmode asynchron
  }

  reset_vars
  set global_after(inner_loop) [after 1000 inner_loop]
  .bar configure -background #00f
}
