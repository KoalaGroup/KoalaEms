# $ZEL: scaler_open_ved.tcl,v 1.3 2006/08/13 17:32:07 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global_comm_ved(idx)         VED for command idx
# global_comm_xved(idx)        VED-command
#

proc open_veds {} {
  global global_comm_ved global_comm_xved

  foreach idx [array names global_comm_ved] {
    set vedname $global_comm_ved($idx)
    set global_comm_xved($idx) ""
    foreach i [ems_openveds] {
      if {[$i name]=="$vedname"} {
        set global_comm_xved($idx) $i
        # output "ved $vedname for command $idx already open"
        break
      }
    }
    if {$global_comm_xved($idx)==""} {
      output "connect to VED $vedname ..."
      update idletasks
      if [catch {set global_comm_xved($idx) [ems_open $vedname]} mist] {
        output_append $mist
        update idletasks
        return -1
      } else {
        output_append "ok."
        update idletasks
      }
    }
  }
  return 0
}
