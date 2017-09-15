#
# global vars in this file:
#
# global_setup
# global_after(id)
# global_after(can_disconnect)
# ems_nocount
#

proc ems_unsol_bye {} {
  global global_after

  output "ems_bye received from commu"
  if {$global_after(can_disconnect)} {
    after cancel $global_after(id)
    ems_disconnect
    after 20000 connect_loop
    output "disconnected; will try to reconnect in 20 seconds"
  }
}

proc open_is {} {
  global global_setup global_veds global_openveds\
      global_ved_of_is global_idx_of_is global_iss

  foreach is $global_setup(iss) {
    set ved $global_ved_of_is($is)
    set vedcomm $global_openveds($ved)
    set idx $global_idx_of_is($is)
    output "open IS $idx on VED [$vedcomm name] ..."
    update idletasks
    if [catch {$vedcomm is open $idx} mist] {
      output_append $mist
      return -1
    } else {
      set global_iss($is) $mist
      output_append ok
    }
  }
  return 0
}

proc init_loop {} {
  global global_after is_display_time is_display_time_2

# nur provisorisch:
  if {[open_is]!=0} {
    return -1
  }
  if {[veds_initialized]==0} {
    #set global_after(can_disconnect) 0
    init_procs
    #set global_after(can_disconnect) 1
    foreach ved [ems_openveds] {
      $ved confmode asynchron
    }
    init_readscaler_loop
    set is_display_time 1
    set is_display_time_2 1
    readscaler_loop
  } else {
    set global_after(can_disconnect) 1
    set global_after(id) [after 20000 init_loop]
  }
  return 0
}

proc vedopen_loop {} {
  global global_after

  set res [open_veds]
  if {$res==0} {
    init_loop
  } elseif {$res==-1} {
    set global_after(can_disconnect) 1
    set global_after(id) [after 20000 vedopen_loop]
  } else {
    exit
  }
}

proc connect_loop {} {
  global ems_nocount global_after

  set ems_nocount 1
  ems_unsolcommand 2 {ems_unsol_bye}
  connect_commu
  if {[ems_connected]} {
    set global_after(can_disconnect) 1
    vedopen_loop
  } else {
    # aller 20 Sekunden ein neuer Versuch
    output "connect without success; will retry after 20 seconds"
    set global_after(id) [after 20000 connect_loop]
  }
}
