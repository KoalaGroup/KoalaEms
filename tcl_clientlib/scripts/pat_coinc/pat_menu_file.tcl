# $ZEL: pat_menu_file.tcl,v 1.1 2002/09/26 12:15:13 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur erzeugt das Filemenu im Hauptmenu
#

proc create_menu_file {parent} {
  set self [string trimright $parent .].file
  menubutton $self -text "File" -menu $self.m -underline 0
  menu $self.m -tearoff 0
  $self.m add command -label "Reread Source" -underline 0\
      -command reset_auto
  $self.m add command -label "Quit" -underline 0 -command ende
  return $self
  }

proc reset_auto {} {
  auto_reset
  namespacedummies
}
