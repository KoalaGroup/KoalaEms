# $Id: objects_changed.tcl,v 1.6 1999/08/07 20:35:48 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global_dataoutlist($ved)     list of all dataouts of ved
# global_dataout_${ved}_${idx}(ved)    vedcommand
# global_dataout_${ved}_${idx}(idx)    is-adx
# global_dataout_${ved}_${idx}(device) pathname if device is tape
# global_dataout_${ved}_${idx}(tape)   device type
# global_dataout_${ved}_${idx}(vendor) device vendor
# global_dataout_${ved}_${idx}(type)   generalized device type
# global_dataout_${ved}_${idx}(istape) 0: no tape 1: is tape
# 
# global_rostatvar_$ved
# global_veds_writing
# global_veds_running
# global_veds_reading
#

proc actnames {action} {
  switch $action {
    0  {set act "none"}
    1  {set act "create"}
    2  {set act "delete"}
    3  {set act "change"}
    4  {set act "start"}
    5  {set act "stop"}
    6  {set act "reset"}
    7  {set act "resume"}
    8  {set act "enable"}
    9  {set act "disable"}
    10 {set act "finish"}
    default {set act "unknown action $action"}
  }
  return $act
}

proc statnames {status} {
  switch $status {
    0  {set act "running"}
    1  {set act "idle"}
    2  {set act "done"}
    3  {set act "error"}
    4  {set act "flushing"}
    default {set act "unknown status $status"}
  }
  return $act
}

proc dom_do_changed {ved idx action} {
  global global_dataoutlist
  if {$action==1} {
    global global_dataout_${ved}_$idx
    lappend global_dataoutlist($ved) $idx
    set global_dataout_${ved}_${idx}(ved) $ved
    set global_dataout_${ved}_${idx}(idx) $idx
    set global_dataout_${ved}_${idx}(istape) 0
    change_status_docreate $ved $idx
    add_do_to_tapedisplay $ved $idx
  } elseif {$action==2} {
    global global_dataout_${ved}_$idx
    delete_do_from_tapedisplay $ved $idx
    change_status_dodelete $ved $idx
    set i [lsearch -exact $global_dataoutlist($ved) $idx]
    if {$i<0} {
      output "dom_do_changed: no dataout $idx for VED $ved" tag_orange
    } else {
      set global_dataoutlist($ved) [lreplace $global_dataoutlist($ved) $i $i]
    }
    unset global_dataout_${ved}_${idx}
  } else {
    output "VED $ved: [actnames $action] domain dataout $idx" tag_blue
  }
}

proc obj_do_changed {ved idx action} {
  global global_rostatvar_$ved
  global global_veds_writing

  if {$idx==-1} {
    if {$action==10} {
      set global_veds_writing($ved) 0
      update idletasks
    } else {
      output "VED $ved: [actnames $action] object dataout $idx" tag_blue
    }
  } else {
    if {($idx>=0) && ($action==10)} {
      status_update_do $ved $idx
    } elseif {$action==1} {
      output "VED $ved: [actnames $action] object dataout $idx" tag_blue
    } elseif {$action==2} {
      output "VED $ved: [actnames $action] object dataout $idx" tag_blue
    } else {
      output "VED $ved: [actnames $action] object dataout $idx" tag_blue
    }
  }
}

proc obj_ro_changed {ved idx action} {
  global global_rostatvar_$ved
  global global_veds_running global_veds_reading

  if {$action==4} {
    set global_veds_running($ved) 1
  } elseif {$action==6} {
    set global_veds_running($ved) 0
  } elseif {$action==10} {
    set global_veds_reading($ved) 0
  } elseif {$action==3} {
    # change
  } else {
    output "VED $ved: [actnames $action] readout" tag_blue
  }
  status_update_ved $ved
}

proc obj_ved_changed {ved action} {
  global global_dataoutlist
  if {$action==6} {
    change_status_vedreset $ved
    foreach idx $global_dataoutlist($ved) {
      delete_do_from_tapedisplay $ved $idx
    }
    set global_dataoutlist($ved) {}
  } else {
    output "VED $ved: [actnames $action]" tag_blue
  }
}
