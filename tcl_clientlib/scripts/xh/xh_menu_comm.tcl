#
# global vars in this file:
#
#

proc create_menu_commands {parent} {
  global global_setup
  
  create_axis_win
  create_data_win
#  create_dataprint_win
  create_datasave_win
  create_datarestore_win

  set self [string trimright $parent .].comm
  menubutton $self -text "Commands" -menu $self.m -underline 0
  menu $self.m

  $self.m add checkbutton -label "Menubar" -underline 0 \
      -accelerator m\
      -offvalue 0 -onvalue 1 -variable global_setup(show_menubar)\
      -command pack_all -selectcolor black

  $self.m add checkbutton -label "Scrollbar" -underline 0 \
      -accelerator s\
      -variable global_setup(show_scrollbar)\
      -command pack_all -selectcolor black

  $self.m add checkbutton -label "Legend" -underline 0 \
      -accelerator l\
      -variable global_setup(show_legend)\
      -command pack_all -selectcolor black

  $self.m add separator

  $self.m add command -label "Jump to end" -underline 0 \
      -command xh_space

  $self.m add separator

  $self.m add command -label "Axes..." -underline 0 \
      -command axis_win_open

  $self.m add command -label "Data..." -underline 0 \
      -command data_win_open

  $self.m add separator

  $self.m add command -label "Print data..." -underline 0 \
      -command dataprint_win_open

  $self.m add command -label "Save data..." -underline 0 \
      -command datasave_win_open

  $self.m add command -label "Restore data..." -underline 0 \
      -command datarestore_win_open

  return $self
}
