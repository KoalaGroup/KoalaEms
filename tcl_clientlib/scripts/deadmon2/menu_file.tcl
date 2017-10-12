# $ZEL: menu_file.tcl,v 1.1 2000/07/15 21:37:01 wuestner Exp $
# copyright:
# 1999 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur erzeugt das Filemenu im Hauptmenu
#

proc create_menu_file {parent} {
  set self [string trimright $parent .].file
  menu $self -title "DeadMon File"

  $self add command -label "Reread Source" -underline 0\
      -command reset_auto

  $self add command -label "Quit" -underline 0 -command prog_ende
  return $self
  }

proc reset_auto {} {
  auto_reset
  namespacedummies
}
