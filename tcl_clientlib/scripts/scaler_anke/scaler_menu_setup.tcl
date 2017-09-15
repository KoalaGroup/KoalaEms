#
# Diese Prozedur erzeugt das Setupmenu im Hauptmenu
#

proc create_menu_setup {parent} {
  set self [string trimright $parent .].setup
  menubutton $self -text "Setup" -menu $self.m -underline 1
  menu $self.m

  $self.m add checkbutton -label "graphical Display" -underline 0 \
      -variable global_setup(graphical_display)\
      -selectcolor black

  $self.m add checkbutton -label "numerical Display" -underline 0 \
      -variable global_setup(numerical_display)\
      -selectcolor black

  $self.m add checkbutton -label "Interval Slider" -underline 0 \
      -variable global_setup(interval_slider)\
      -selectcolor black -command repack_all

  $self.m add separator

  $self.m add command -label "Commu ..." -underline 0 \
      -command commu_win_open

  $self.m add command -label "xh ..." -underline 0 \
      -command xh_win_open

  $self.m add command -label "Options ..." -underline 0 \
      -command option_win_open

  $self.m add command -label "Channels ..." -underline 0 \
      -command channel_win_open

  $self.m add command -label "Print ..." -underline 0 \
      -command print_win_open

  $self.m add separator

  $self.m add command -label "Setup File ..." -underline 0 \
      -command setupfile_win_open

  $self.m add checkbutton -label "Autosave" -underline 0 \
      -variable global_setup(auto_save)\
      -selectcolor black

  $self.m add command -label "Save Setup" -underline 0\
      -command {save_setup_default}

  $self.m add command -label "Restore Setup" -underline 0\
      -command {restore_setup_default}

  return $self
  }
