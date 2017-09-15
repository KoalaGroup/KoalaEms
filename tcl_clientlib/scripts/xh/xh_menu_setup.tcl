#
# global vars in this file:
#
# boolean show_menubar
#

proc create_menu_setup {parent} {
  create_autosave_win
  create_autostart_win

  set self [string trimright $parent .].setup
  menubutton $self -text "Setup" -menu $self.m -underline 0
  menu $self.m

  $self.m add command -label "Setup File ..." -underline 0 \
      -command setupfile_win_open

  $self.m add checkbutton -label "Autosave" -underline 0 \
      -variable global_setup(autosave)\
      -selectcolor black

  $self.m add command -label "Save Setup" -underline 0\
      -command {save_setup_default}

  $self.m add command -label "Restore Setup" -underline 0\
      -command {restore_setup_default}

  $self.m add separator

  $self.m add command -label "Autosave Data..." -underline 0 \
      -command autosave_win_open

  $self.m add command -label "Autostart..." -underline 0 \
      -command autostart_win_open

  return $self
}
