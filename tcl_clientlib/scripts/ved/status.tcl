#
# global vars in this file:
#
# array  global_setup
# string global_status(modus)
# string global_status(commu)
# string global_status(ved)
# string global_status(is)
#

proc update_status {} {
  global global_setup global_status
  if $global_setup(offline) {
    set global_status(modus) {offline}
  } else {
    set global_status(modus) {online}
  }
  set global_status(commu) [ems_connection]
  set global_status(ved) {}
  set global_status(is) {}
}

proc create_status {parent} {
  global global_status
  update_status

  set self [string trimright $parent .].status
  frame $self -relief raised -borderwidth 2
  frame $self.lframe -relief sunken -borderwidth 1
  frame $self.rframe -relief sunken -borderwidth 1
#  -width 500

  label $self.lframe.modus -text "Modus:" -anchor w
  label $self.rframe.modus -textvariable global_status(modus) -anchor w

  label $self.lframe.commu -text "Commu:" -anchor w
  label $self.rframe.commu -textvariable global_status(commu) -anchor w

  label $self.lframe.ved -text "VED:" -anchor w
  label $self.rframe.ved -textvariable global_status(ved) -anchor w

  label $self.lframe.is -text "IS:" -anchor w
  label $self.rframe.is -textvariable global_status(is) -anchor w

  pack $self.lframe.modus $self.lframe.commu $self.lframe.ved $self.lframe.is\
      -side top -fill x
  pack $self.rframe.modus $self.rframe.commu $self.rframe.ved $self.rframe.is\
      -side top -fill x -expand 1
  pack $self.lframe -side left -fill y
  pack $self.rframe -side left -fill both -expand 1
}
