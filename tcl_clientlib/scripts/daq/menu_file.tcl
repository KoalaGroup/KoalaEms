# $ZEL: menu_file.tcl,v 1.8 2003/02/04 19:27:54 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur erzeugt das Filemenu im Hauptmenu
#

proc create_menu_file {parent} {
  set self [string trimright $parent .].file
  menu $self -title "DAQ File"

  $self add command -label "Setup File ..." -underline 0 \
      -command setupfile_win_open

  $self add command -label "Save Setup" -underline 0\
      -command {save_setup_default}

  $self add command -label "Restore Setup" -underline 0\
      -command {restore_setup_default}

  $self add separator

  $self add command -label "Reread Source" -underline 0\
      -command reset_auto

  $self add command -label "Quit" -underline 0 -command daq_ende
  return $self
  }

proc reset_auto {} {
  auto_reset
  namespacedummies
}
