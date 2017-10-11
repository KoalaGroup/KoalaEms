# $ZEL: scaler_callbacks.tcl,v 1.4 2011/11/27 00:08:43 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_comm_callback(idx) callbacklist
# global_comm_num(idx)      number of channels
# global_comm_offs(idx)     index of first channel
# global_comm_sequence      sequence number
# global_comm_reqnum(soll)  number of requests per sequence
# global_comm_reqnum(ist)   number of requests per sequence
#

proc callback {sequence idx args} {
  global global_comm_callback global_comm_offs
  global global_comm_sequence global_comm_reqnum
  global ro_status global_num_channels

  set jetzt [doubletime]
  set rest $args
  set offs $global_comm_offs($idx)
  foreach proc_num $global_comm_callback($idx) {
    set proc [lindex $proc_num 0]
    set num [lindex $proc_num 1]
    set res [$proc $idx $offs $num $jetzt rest]
    if {$res!=0} {
      output "abgebrochen"
      return
    }
    incr offs $num
  }

  if {"$ro_status(status)" == "$ro_status(status_disp)"} {
    set immer 0
  } else {
    set immer 1
  }
  if {$sequence==$global_comm_sequence} {
    incr global_comm_reqnum(ist)
    if {($global_comm_reqnum(ist)==$global_comm_reqnum(soll))||$immer} {
      update_display $jetzt $immer
      send_to_net 0 $global_num_channels $jetzt
    }
  } else {
    update_display $jetzt $immer
    send_to_net 0 $global_num_channels $jetzt
  }
}

proc error_callback {sequence idx conf} {
  #output "error_callback idx=$idx; [$conf print]"
  $conf delete
}
