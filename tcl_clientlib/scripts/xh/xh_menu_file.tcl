proc create_menu_file {parent} {
  set self [string trimright $parent .].file
  menubutton $self -text "File" -menu $self.m -underline 0
  menu $self.m -tearoff 0
  $self.m add command -label "Reread Source" -underline 0\
      -command auto_reset
  $self.m add command -label "Quit" -underline 0 -command xh_end
  return $self
  }
