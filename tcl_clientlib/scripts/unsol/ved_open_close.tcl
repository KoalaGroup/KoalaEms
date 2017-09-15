proc open_ved {ved} {
  global dialog_nr global_setup
  if {[lsearch $global_setup(veds) $ved]>=0} {
    set win .error_$dialog_nr
    incr dialog_nr
    tk_dialog $win Error "VED $ved is already open or scheduled for opening" \
        {} 0 Dismiss
  } else {
    lappend global_setup(veds) $ved
    set global_setup(veds) [lsort -ascii $global_setup(veds)]
    ved_close_menu_change
    output "ved $ved scheduled for open"
    restart_main_loop
  }
}

proc close_ved {ved} {
  global global_setup
  set openveds [ems_openveds]
  foreach v $openveds {
    if {[$v name]==$ved} {
      $v close
      output "$v closed"
    }
  }
  set idx [lsearch $global_setup(veds) $ved]
  if {$idx>=0} {
    set global_setup(veds) [lreplace $global_setup(veds) $idx $idx]
    # output "ved $ved deleted from list"
  } else {
    output "Warning: close_ved $ved: ved not found in \$global_setup(veds)"
  }
  ved_close_menu_change
}
