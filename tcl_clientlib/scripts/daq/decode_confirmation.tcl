# $ZEL: decode_confirmation.tcl,v 1.2 2010/08/04 21:08:21 wuestner Exp $
# copyright 2009
#   Peter Wuestner;
#   Zentralinstitut fuer Elektronik; Forschungszentrum Juelich GmbH
#

proc dump_proclist {proclist idx} {
    set i 1
    while {[llength $proclist]} {
        if {$i==$idx} {
            set text {==> }
        } else {
            set text {    }
        }
        append text [format {%2d %s %s} $i [lindex $proclist 0] \{[lindex $proclist 1]\}]
        output_append $text tag_red
        set proclist [lrange $proclist 2 end]
        incr i
    }
}

proc decode_error_confirmation {name conf proclist} {
    global reqtypearr reqnamearr

    #append space ved_setup_ $name
    #set vedname [$space eval ved name]
    output_append "[$conf print text]" tag_red
    #output_append "[$conf print tabular]" tag_green
    set reqtype [$conf get reqtype]
    if {$reqtype!=$reqnamearr(DoCommand)} {
        output_append "unexpected reqtype $reqtype" tag_red
        output_append "[$conf print tabular]" tag_red
        return
    }
    set error [$conf get body 0]
    switch $error {
        31 {
            output_append "error in procedure [$conf get body 1]:" tag_red
            output_append "  [ems_plcode [$conf get body 2]]" tag_red
            dump_proclist $proclist [$conf get body 1]
            set num [$conf get body 3]
            if {[$conf get body 3]} {
                output_append "  additional data: [lrange [$conf get body] 4 end]" tag_red
            } else {
                output_append "  no additional data provided" tag_red
            }
        }
        32 {
            set body [$conf get body]
            set error [lindex $body end]
            set proc [lindex $body end-1]
            set data [lrange $body 1 end-2]
            output_append "error in procedure $proc:" tag_red
            output_append "  [ems_plcode $error]" tag_red
            dump_proclist $proclist $proc
            if {[llength $data]} {
                output_append "  additional data: $data" tag_red
            } else {
                output_append "  no additional data provided" tag_red
            }
        }
        default {
            output_append "[$conf print tabular]" tag_red
            dump_proclist $proclist [$conf get body 0]
        }
    }

}
