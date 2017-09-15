#
# global vars in this file:
#
# window global_menu(win)
#

proc create_menu {parent} {
  global global_menu
  set self [string trimright $parent .].menu
  set global_menu(win) $self
  frame $self -relief ridge -borderwidth 3
  set filemenu [create_menu_file $self]
  set setupmenu  [create_menu_setup $self]
  set helpmenu [create_menu_help $self]
  pack $filemenu $setupmenu -side left -padx 1m
  pack $helpmenu -side right -anchor e
  return $self
}
