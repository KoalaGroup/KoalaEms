#
# Diese Prozedur erzeugt das Setupmenu im Hauptmenu
#

proc create_menu_setup {parent} {
  set self [string trimright $parent .].setup
  menu $self -title "DeadMon Setup"

  $self add command -label "Commu ..." -underline 0 \
      -command commu_win_open

  $self add separator

  $self add command -label "Setup File ..." -underline 0 \
      -command setupfile_win_open

  $self add checkbutton -label "Autosave" -underline 0 \
      -variable global_setup(auto_save)\
      -selectcolor black

  $self add checkbutton -label "Autostart" -underline 0 \
      -variable global_setup(auto_start)\
      -selectcolor black

  $self add command -label "Save Setup" -underline 0\
      -command {save_setup_default}

  $self add command -label "Restore Setup" -underline 0\
      -command {restore_setup_default}

  return $self
  }
