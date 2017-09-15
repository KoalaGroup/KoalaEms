# $Id: menu_file.tcl,v 1.1 2000/07/15 21:34:38 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# Diese Prozedur erzeugt das Filemenu im Hauptmenu
#

proc create_menu_file {parent} {
  set self [string trimright $parent .].file
  menu $self -title "WINCC File"

  $self add command -label "Setup File ..." -underline 0 \
      -command setupfile_win_open

  $self add command -label "Save Setup" -underline 0\
      -command {varframe::mk_setup; save_setup_default}

  $self add command -label "Restore Setup" -underline 0\
      -command {restore_setup_default}

  $self add separator

  $self add command -label "Reread Source" -underline 0\
      -command reset_auto

#   $self add command -label "Test bgerror" -underline 0\
#       -command {output [info body bgerror] tag_green}

  $self add command -label "Quit" -underline 0 -command ende
  return $self
  }

proc reset_auto {} {
  auto_reset
  dummies
  bind Entry <Delete> {tkEntryBackspace %W}
}
