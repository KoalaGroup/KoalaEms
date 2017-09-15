# $Id: menu_setup.tcl,v 1.1 2000/07/15 21:34:39 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# Diese Prozedur erzeugt das Setupmenu im Hauptmenu
#

proc create_menu_setup {parent} {
  set self [string trimright $parent .].setup
  menu $self -title "WINCC Setup"

  $self add command -label "Commu ..." -underline 0 \
      -command commu_win_open

  $self add command -label "VED ..." -underline 0 \
      -command ved_setup::win_open

  $self add command -label "WINCC host ..." -underline 0 \
      -command wincc_setup::win_open

  return $self
  }
