# $ZEL: unsol_ved.tcl,v 1.10 2003/10/20 09:55:35 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc unsol_any {space v h d} {
    if ([lindex $h 3]==14) {
        unsol_devicedisconnect $space $v $h $d
        return
    }
  output "VED [$v name]: unsol. message" tag_orange
  output_append "   header: $h"  tag_orange
  output_append "   data:   $d"  tag_orange
}
