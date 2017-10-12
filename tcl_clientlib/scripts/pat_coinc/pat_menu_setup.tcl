# $ZEL: pat_menu_setup.tcl,v 1.1 2002/09/26 12:15:13 wuestner Exp $
# P. Wuestner 2002; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# Diese Prozedur erzeugt das Setupmenu im Hauptmenu
#

proc create_menu_setup {parent} {
  set self [string trimright $parent .].setup
  menubutton $self -text "Setup" -menu $self.m -underline 1
  menu $self.m

  $self.m add command -label "Interval ..." -underline 0 \
      -command interval_win_open

  $self.m add command -label "Commu ..." -underline 0 \
      -command commu_win_open

  $self.m add separator

  $self.m add command -label "Save Setup" -underline 0\
      -command {save_setup_default}

  $self.m add command -label "Restore Setup" -underline 0\
      -command {restore_setup_default}

  return $self
  }
