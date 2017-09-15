# $ZEL: menu_setup.tcl,v 1.5 2003/02/04 19:27:54 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur erzeugt das Setupmenu im Hauptmenu
#

proc create_menu_setup {parent} {
  set self [string trimright $parent .].setup
  menu $self -title "DAQ Setup"

  $self add command -label "Commu ..." -underline 0 \
      -command commu_win_open

  $self add command -label "Master Setup File ..." -underline 0 \
      -command supersetupfile::win_open

  return $self
  }
