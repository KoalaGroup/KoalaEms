
proc ved_close_menu_change {} {
  global global_closemenu
  $global_closemenu delete 0 end
  set list [lsort [ems_openveds]]
  foreach i $list {
    regsub ^ved_ $i {} label 
    $global_closemenu add command -label $label -command "close_ved $i"
  }
}

proc create_menu_ved {parent} {
  global global_closemenu

  set self [string trimright $parent .].ved
  menu $self -title "DeadMon VED"
  $self add command -label "Open ..." -underline 0 \
      -command VED::win_open

  set global_closemenu [menu $self.close -title "DeadMon close VED"]
  $self add cascade -label "Close" -underline 0 \
      -menu $global_closemenu

  return $self
  }
