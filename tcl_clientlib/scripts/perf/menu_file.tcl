#
# Diese Prozedur erzeugt das Filemenu im Hauptmenu
#

proc create_menu_file {parent} {
  set self [string trimright $parent .].file
  menu $self -title "EMSperf File"
  $self add command -label "Reread Source" -underline 0\
      -command reset_auto
  $self add command -label "Quit" -underline 0 -command ende
  return $self
  }

proc reset_auto {} {
  global sources_with_ns
  auto_reset
#   foreach i $sources_with_ns {
#     source $i
#   }
}
