# $ZEL: unsol_devicedisconnect.tcl,v 1.2 2004/02/25 11:53:10 wuestner Exp $
# copyright 2003
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

# typedef enum {
#   modul_none, modul_unspec, modul_generic, modul_camac,
#   modul_fastbus, modul_vme, modul_lvd, modul_invalid
# } Modulclass;

proc unsol_devicedisconnect {space v h d} {
    set cratetype_num [lindex $d 0]
    set crate [lindex $d 1]
    set disco [expr [lindex $d 2]<0]
    switch $cratetype_num {
        3 {set cratetype CAMAC}
        4 {set cratetype FASTBUS}
        5 {set cratetype VME}
        6 {set cratetype LVD}
        default {set cratetype "(unknown type $cratetype_num)"}
    }
    set initialised [lindex $d 3]

    if {$disco} {
        set tag tag_red
        set action disconnected
    } else {
        if {$initialised} {
            set tag tag_orange
        } else {
            set tag tag_red
        }
        set action {(re-)connected}
    }

    output "VED [$v name] ([ved_setup_$space eval set description]):" $tag
    output_append "  Crate $cratetype $crate $action" $tag
    if {!$disco} {
        if {$initialised} {
            output_append "  The crate was only disconnected, not switched off" $tag
        } else {
            output_append {  The Crate was switched off; please press "INIT"} $tag
        }
    }
}
