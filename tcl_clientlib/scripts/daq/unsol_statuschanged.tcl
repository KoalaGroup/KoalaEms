# $ZEL: unsol_statuschanged.tcl,v 1.11 2011/04/12 22:50:15 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc unsol_statuschanged {space v h d} {
    global global_daq
    set action [lindex $d 0]
    set object [lindex $d 1]

# objects:
#  0 null
#  1 VED
#  2 domain
#    0 null
#    1 Modullist
#    2 LAMproclist
#    3 Trigger
#    4 Event
#    5 Datain,
#    6 Dataout
#  3 IS
#  4 variable
#  5 PI
#    0 null
#    1 readout
#    2 LAM
#  6 dataout

    switch $action {
        0 {
            set action_name none
        }
        1 {
            set action_name create
        }
        2 {
            set action_name delete
        }
        3 {
            set action_name change
        }
        4 {
            set action_name start
        }
        5 {
            set action_name stop
        }
        6 {
            set action_name reset
        }
        7 {
            set action_name resume
        }
        8 {
            set action_name enable
        }
        9 {
            set action_name disable
        }
        10 {
            set action_name finish
        }
        11 {
            set action_name file_created
        }
        default {
            set action_name $action
        }
    }

    print_unsol_statuschanged $v $d
    switch $object {
        1 {
            # Object_ved
            #puts "obj_ved_changed $action_name"
            obj_ved_changed $v $action
        }
        2 {
            # Object_domain
            #puts "obj_ved_changed $action_name"
            if {[lindex $d 2]==6} {
                dom_do_changed $v [lindex $d 3] $action
            }
        }
        3 {
            # Object_is
            #puts "obj_is_changed $action_name"
        }
        4 {
            # Object_var
            #puts "obj_var_changed $action_name"
        }
        5 {
            # Object_pi
            #puts "obj_pi_changed $action_name"
            if {[lindex $d 2]==1} {
                obj_ro_changed $v [lindex $d 3] $action
            }
        }
        6 {
            # Object_do
            #puts "obj_do_changed $action_name"
            if {$action==11} {
                #display_do_filename $v [lindex $d 2] [lrange $d 3 end]
                output "filename: [lrange $d 3 end]" tag_blue
                xdr2string [lrange $d 3 end] name
                set global_daq(filename) $name
            } else {
                obj_do_changed $v [lindex $d 2] $action
            }
        }
        default {
            output_append "  unknown object $object ($action_name)" tag_orange
            output_append "  v=$v" tag_orange
            foreach i $v {
                output_append "    $i"
            }
            output_append "  h=$h" tag_orange
            foreach i $h {
                output_append "    [format {%08x} $i]"
            }
            output_append "  d=$d" tag_orange
            foreach i $d {
                output_append "    [format {%08x} $i]"
            }
        }
    }
}
