# if $global_setup(evrate_ved)!=""
#   $global_openveds($global_setup(evrate_ved)) readout status : readout_ok ? readout_error
#
# foreach is $global_setup(iss)
#   $global_openveds(global_is_$is(ved)) 

proc init_procs {} {
  global global_setup global_iss global_openveds
  global global_memberprocs_of_is global_memberprocs2_of_is
  global global_callback_of_is global_err_of_is
  global evrate_proc scaler_proc scaler_proc2
  global global_ved_of_is global_idx_of_is

  set evrate_proc "$global_openveds($global_setup(evrate_ved)) \
      readout status 2 : got_evrate ? err_evrate"
  foreach is $global_setup(iss) {
    set scaler_proc($is) "$global_iss($global_idx_of_is($is)) \
    $global_memberprocs_of_is($is) : $global_callback_of_is($is) ? $global_err_of_is($is)"
#    set scaler_proc2($is) "$global_openveds($global_ved_of_is($is)) \
#    $global_memberprocs2_of_is($is) : got_scalerate2 ? err_scalerate2"
  }
}
