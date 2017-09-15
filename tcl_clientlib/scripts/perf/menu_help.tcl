#
# Diese Prozedur erzeugt das Setupmenu im Hauptmenu
#

proc create_menu_help {parent} {
  set self [string trimright $parent .].help
  menu $self -tearoff 0
  $self add command -label "Hilfe!!" -underline 0
  return $self
  }
