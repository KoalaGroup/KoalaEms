# $ZEL: unsol_text.tcl,v 1.2 2007/04/15 13:15:09 wuestner Exp $
# copyright 2004
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc unsol_text {space v h d} {
    output "Message from [$v name]:" tag_orange
    #output_append "   header: $h"  tag_orange
    set lines [lindex $d 0]
    set d [lrange $d 1 end]
    for {} {$lines>0} {incr lines -1} {
        set d [xdr2string $d t]
        output_append "   $t"  tag_orange
    }
}
