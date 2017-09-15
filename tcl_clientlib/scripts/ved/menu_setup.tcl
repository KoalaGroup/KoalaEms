#
# global vars in this file:
#
# global_setup(offline)
#

proc create_menu_setup {parent} {
  global global_setup
  set self [string trimright $parent .].opt
  menubutton $self -text "Setup" -menu $self.m -underline 0
  menu $self.m

  if {[info exists global_setup(offline)] == 0} {
    set global_setup(offline) 1
  }
  if {[info exists global_setup(autosave)] == 0} {
    set global_setup(autosave) 0
  }
  $self.m add checkbutton -label "offline" -underline 0 \
      -variable global_setup(offline)\
      -selectcolor black\
      -command update_status

  $self.m add checkbutton -label "autosave" -underline 0 \
      -variable global_setup(autosave)\
      -selectcolor black

  $self.m add command -label "Communication..." -underline 0 \
      -command commu_win_open

  $self.m add command -label "Open VED..." -underline 0 \
      -command ved_win_open

  return $self
}
