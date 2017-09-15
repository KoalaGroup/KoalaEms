# $Id: correct_setup.tcl,v 1.2 2005/04/06 19:28:46 wuestner Exp $
#

proc correct_setup {} {
  global global_setup

  if {![info exists global_setup(maintitle)]} {
    set global_setup(maintitle) "Discriminatorsetup"}

  if {![info exists global_setup(shorttitle)]} {
    set global_setup(shorttitle) "Discri"}

  if {![info exists global_setup(crate)]} {
    set global_setup(crate) 0}
}
