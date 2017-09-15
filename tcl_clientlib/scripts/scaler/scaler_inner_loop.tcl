# $Id: scaler_inner_loop.tcl,v 1.1 1998/09/10 18:35:05 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_setup(readout_repeat)
# global_after(outer_loop) (?)
# global_after(inner_loop)
# global_comm_indices       indices of command definitions
# global_comm_xved($idx)
# global_comm_xis($idx)
# global_comm_proc($idx)
# global_comm_command($idx)
# global_comm_callback($idx)
# global_comm_sequence      sequence number
# global_comm_reqnum(ist)   number of requests per sequence
# 
#

proc inner_loop {} {
  global global_setup
  global global_after
  global global_comm_indices
  global global_comm_xved
  global global_comm_xis
  global global_comm_proc
  global global_comm_command
  global global_comm_callback
  global ems_debug
  global global_comm_reqnum
  global global_comm_sequence
  
  incr global_comm_sequence
  set global_comm_reqnum(ist) 0
  if [catch {
    foreach idx $global_comm_indices {
      set index $idx
      if $global_comm_proc($idx) {
        set kommando "$global_comm_xis($idx) command $global_comm_command($idx) : callback $global_comm_sequence $idx ? error_callback $global_comm_sequence $idx"
        $global_comm_xis($idx) command $global_comm_command($idx) : callback $global_comm_sequence $idx ? error_callback $global_comm_sequence $idx
      } else {
        set kommando "$global_comm_xved($idx) $global_comm_command($idx) : callback $global_comm_sequence $idx ? error_callback $global_comm_sequence $idx"
        eval $global_comm_xved($idx) $global_comm_command($idx) : callback $global_comm_sequence $idx ? error_callback $global_comm_sequence $idx
      }
    }
  } mist] {
    output "error in execution of command $index"
    output_append "command was: $kommando"
    output_append $mist
    output_append "will restart in 120 seconds"
    disconnect_commu
    set global_after(outer_loop) [after 120000 outer_loop]
    return
  }
  set global_after(inner_loop) [after $global_setup(readout_repeat) inner_loop]
}
