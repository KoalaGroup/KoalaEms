# $ZEL: scaler_open_is.tcl,v 1.5 2016/10/11 21:38:34 wuestner Exp $
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

proc make_islist {ved} {
    global global_islist

    output "making islist for $ved"
    update idletasks
    set global_islist($ved) {}
    set idxlist [$ved namelist is]
    output_append "idx list is $idxlist"
    update idletasks
    foreach idx $idxlist {
        set id [$ved is id $idx]
        lappend global_islist($ved) "$id $idx"
    }
    output_append "full list is $global_islist($ved)"
    return 0
}

proc open_ISs_by_ID {} {
    global global_comm_indices
    global global_comm_proc
    global global_comm_xved
    global global_comm_is
    global global_comm_xis
    global global_islist

    foreach idx $global_comm_indices {
# Also commands can (and want) use ISs. IS 0 is identical to pure VED.
#        # only if a ems procedure is used, ems commands do not need an IS
#        if {$global_comm_proc($idx)} {
            set ved $global_comm_xved($idx)
            set isid $global_comm_is($idx)
            if {![info exists global_islist($ved)]} {
                make_islist $ved
            }
            # find the IS
            set isidx -1
            foreach is $global_islist($ved) {
                if {[lindex $is 0]==$isid} {
                    set isidx [lindex $is 1]
                    break
                }
            }
            if {$isidx<0} {
                output "IS $isid does not exist in $ved"
                update idletasks
                return -1
            }
            # is it already open?
            set global_comm_xis($idx) ""
            foreach is [$ved openis] {
                if {[$is idx]==$isidx} {
                    set global_comm_xis($idx) $is
                    output "IS $isidx for command $idx already open"
                    update idletasks
                    break
                }
            }
            if {$global_comm_xis($idx)==""} {
                output "open IS $isidx (ID $isid) of VED [$ved name] ..."
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
#        }
    }
    return 0
}

proc open_ISs_by_IDX {} {
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
          update idletasks
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

proc open_is {} {
    global global_setup
    if {$global_setup(use_isid)} {
        return [open_ISs_by_ID]
    } else {
        return [open_ISs_by_IDX]
    }
}
