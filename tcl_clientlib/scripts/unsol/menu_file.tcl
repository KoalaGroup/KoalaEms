#
# Diese Prozedur erzeugt das Filemenu im Hauptmenu
#

proc create_menu_file {parent} {
  set self [string trimright $parent .].file
  menu $self -title "EMSunsol File"
  $self add command -label "Reread Source" -underline 0\
      -command reset_auto
  $self add command -label "Quit" -underline 0 -command unsol_ende
  return $self
  }

proc reset_auto {} {
  auto_reset
  # proc dummytool is called in order to reload bgerror and unsol_ende
  dummytool
}
