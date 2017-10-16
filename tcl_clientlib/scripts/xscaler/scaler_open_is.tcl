# $ZEL: scaler_open_is.tcl,v 1.3 2006/08/13 17:32:07 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars in this file:
#
# global_comm_indices       indices of command definitions
# global_comm_proc(idx)     0: EMS-command 1: procedure list
# global_comm_xved(idx)     VED-command for command idx
# global_comm_is(idx)       IS
# global_comm_xis(idx)      IS-command
# 

proc open_is {} {
  global global_comm_indices
  global global_comm_proc
  global global_comm_xved
  global global_comm_is
  global global_comm_xis

  foreach idx $global_comm_indices {
    if {$global_comm_proc($idx)} {
      set ved $global_comm_xved($idx)
      set isidx $global_comm_is($idx)
      set global_comm_xis($idx) ""
      foreach is [$ved openis] {
        if {[$is idx]==$isidx} {
          set global_comm_xis($idx) $is
          output "IS $isidx for command $idx already open"
          break
        }
      }
      if {$global_comm_xis($idx)==""} {
        output "open IS $isidx of VED [$ved name] ..."
        update idletasks
        if [catch {set global_comm_xis($idx) [$ved is open $isidx]} mist] {
          output_append $mist
          update idletasks
          return -1
        } else {
          output_append "ok."
          update idletasks
        }
      }
    }
  }
  return 0
}
