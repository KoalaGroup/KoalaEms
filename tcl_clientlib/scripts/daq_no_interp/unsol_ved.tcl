# $Id: unsol_ved.tcl,v 1.8 2000/08/31 15:43:12 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc unsol_any {space v h d} {
  output "VED [$v name]: unsol. message" tag_orange
  output_append "   header: $h"  tag_orange
  output_append "   data:   $d"  tag_orange
}
