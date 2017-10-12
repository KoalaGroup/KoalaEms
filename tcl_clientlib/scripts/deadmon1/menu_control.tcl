
proc ved_close_menu_change {} {
  global global_closemenu
  $global_closemenu delete 0 end
  set list [lsort [ems_openveds]]
  foreach i $list {
    regsub ^ved_ $i {} label 
    $global_closemenu add command -label $label -command "close_ved $i"
  }
}

proc create_menu_control {parent} {

  set self [string trimright $parent .].control
  menu $self -title "DeadMon Control"
  $self add command -label "Reset" -underline 0 \
      -command LOOP::clear

  return $self
  }
