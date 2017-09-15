#
# global vars in this file:
#
# none
#

proc create_menu_help {parent} {
  set self [string trimright $parent .].help
  menubutton $self -text "Help" -menu $self.m -underline 0
  menu $self.m -tearoff 0
  $self.m add command -label "???" -underline 0
  return $self
}
