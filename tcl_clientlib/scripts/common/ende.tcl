#$Id: ende.tcl,v 1.3 2002/09/26 12:15:10 wuestner Exp $
#
# Diese Prozedur wird am Programmende aufgerufen, sie ruft save_setup auf,
# wenn global_setup(autosave) auf true steht.
#

proc ende {} {
  global global_setup global_setup_file

  if [info exists global_setup(auto_save)] {
    if {$global_setup(auto_save)} {save_setup_default}
  }
  exit
}
