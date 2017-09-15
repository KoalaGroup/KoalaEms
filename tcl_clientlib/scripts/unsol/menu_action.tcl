#
# Diese Prozedur erzeugt das Actionmenu im Hauptmenu
#

proc start_stop_menu_change {} {
  global global_main_loop global_start_stop_menu
  
  if {$global_main_loop(running)} {
    $global_start_stop_menu(menu) entryconfigure \
        $global_start_stop_menu(idx) -label "Stop Loop"
  } else {
    $global_start_stop_menu(menu) entryconfigure \
        $global_start_stop_menu(idx) -label "Start Loop"
  }
}

proc start_stop_loop {} {
  global global_main_loop
  
  if {$global_main_loop(running)} {
    stop_main_loop
  } else {
    start_main_loop
  }
}

proc create_menu_action {parent} {
  global global_start_stop_menu

  set self [string trimright $parent .].action
  menu $self -title "EMSunsol Action"

  $self add command -command {start_stop_loop}
  set global_start_stop_menu(menu) $self
  set global_start_stop_menu(idx) 1
  start_stop_menu_change

  $self add command -label "Delimiter" -underline 0 \
      -command write_delimiter

  return $self
  }
