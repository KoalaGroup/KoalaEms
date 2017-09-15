proc ved_close_menu_change {} {
  global global_closemenu global_setup
  $global_closemenu delete 0 end
  foreach i $global_setup(veds) {
    $global_closemenu add command -label $i -command "close_ved $i"
  }
}

proc create_menu_ved {parent} {
  global global_closemenu global_start_stop_menu

  set self [string trimright $parent .].ved
  menu $self -title "EMSunsol VED"

  $self add command -label "Open ..." -underline 0 \
      -command ved_win_open

  set global_closemenu [menu $self.close -title "EMSunsol close"]
  $self add cascade -label "Close" -underline 0 \
      -menu $global_closemenu
  ved_close_menu_change
  
  return $self
  }
