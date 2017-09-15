# $Id: menu_file.tcl,v 1.1 2000/03/24 17:13:22 wuestner Exp $
#

proc create_menu_file {parent} {
  global global_setup
  set self [string trimright $parent .].file
  menu $self -title "$global_setup(shorttitle) File"

  $self add command -label "Save Setup" -underline 0\
      -command {save_setup_default}

  $self add command -label "Restore Setup" -underline 0\
      -command {restore_setup_default}

  $self add separator

  $self add command -label "Reread Source" -underline 0\
      -command reset_auto

  $self add command -label "Quit" -underline 0 -command ende_
  return $self
  }

proc reset_auto {} {
  auto_reset
  namespacedummies
}
