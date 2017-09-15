#
# global vars in this file:
#
# global_setup_file
#

proc create_menu_file {parent} {
  global global_setup_file

  set self [string trimright $parent .].file
  menubutton $self -text "File" -menu $self.m -underline 0
  menu $self.m -tearoff 1
  $self.m add command -label "Save Setup" -underline 0\
      -command "save_setup $global_setup_file"
  $self.m add command -label "Restore Setup" -underline 0\
      -command "restore_setup $global_setup_file"
  $self.m add command -label "Save Named Setup..." -underline 5\
      -command "save_setup $global_setup_file"
  $self.m add command -label "Restore Named Setup..." -underline 10\
      -command "restore_setup $global_setup_file"
  $self.m add separator
  $self.m add command -label "Reset autoload" -command auto_reset
  $self.m add separator
  $self.m add command -label "Exit" -underline 0 -command good_bye
  return $self
}
