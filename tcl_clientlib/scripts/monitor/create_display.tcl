# $Id: create_display.tcl,v 1.1 1998/09/13 19:19:28 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_setup(veds)
# global_status($ved)
# global_count($ved)
# 

#-----------------------------------------------------------------------------#
proc create_display {} {
  global global_setup
  global global_status global_count
  
  set row 0
  foreach ved $global_setup(veds) {
    label .disp.name_$ved -text $ved -anchor w -relief ridge -padx 5 -pady 2
    label .disp.stat_$ved -textvariable global_status($ved) -anchor w -relief ridge -padx 5 -pady 2
    label .disp.count_$ved -textvariable global_count($ved) -anchor e -relief ridge -padx 5 -pady 2
    grid .disp.name_$ved .disp.stat_$ved .disp.count_$ved -sticky ewns
    incr row
  }
}
#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
