# $ZEL: epics.tcl,v 1.2 2017/11/08 00:17:20 wuestner Exp $
# copyright 2017
#   Peter Wuestner;
#       Forschungszentrum Juelich
#           Central Institute of Engineering, Electronics and Analytics
#               Electronic Systems (ZEA-2)

# keywords currently recognised are:
#     runnr     ;# set in start.tcl:start_button and start.tcl:auto_restart
#                reset in stop.tcl:stop_button and stop.tcl:automatic_stop
#     filename  ;# set in unsol_statuschanged.tcl:unsol_statuschanged
#                reset in stop.tcl:stop_button and stop.tcl:automatic_stop
#     eventnr   ;# set in update_status.tcl:got_ro_status
#                reset in start.tcl:start_button and start.tcl:auto_restart
#     eventrate ;# set in update_status.tcl:got_ro_status
#                reset in stop.tcl:stop_button and stop.tcl:automatic_stop
#                  and in update_status.tcl:got_ro_status
#                  and in start.tcl:start_button and start.tcl:auto_restart
#     datarate  ;# update_status.tcl:got_do_status stop.tcl
#                reset in stop.tcl:stop_button and stop.tcl:automatic_stop
#                  and in update_status.tcl:got_do_status
#                  and in start.tcl:start_button and start.tcl:auto_restart
#     filesize  ;# update_status.tcl:got_do_status
#                reset in start.tcl:start_button and start.tcl:auto_restart
#
# the variables epics_notification(<keyword>) and
# notification_source(<keyword>) have to be set in <mastersetup>.ved


# epics_put sets the EPICS variable given in epics_notification(keyword)

proc epics_put {keyword val unittype} {

    if {![interp eval mastersetup info exists epics_notification($keyword)]} {
        # output "epics_notification($keyword) does not exist" tag_red
        return
    }

    set epics_var [interp eval mastersetup set epics_notification($keyword)]
    # output "epics_notification($keyword): $epics_var" tag_blue

    if {[string length $epics_var]==0} {
        return
    }

    # do we have to apply a unit?
    if {![string equal $unittype none]} {
        if {[interp eval mastersetup info exists notification_unit($keyword)]} {
            set unittxt [interp eval mastersetup set notification_unit($keyword)]
            switch $unittype {
                size {set unit [parse_unit_size $unittxt]}
                rate {set unit [parse_unit_rate $unittxt]}
                time {set unit [parse_unit_time $unittxt]}
                default {
                    output "illegal unittype $unittype (program error)"
                    set unit 0
                }
            }
            if {$unit==0} {
                return
            }
            set val [expr double($val)/$unit]
        }
    }

    # output "executing caput $epics_var $val" tag_blue
    if {[catch {exec -ignorestderr caput $epics_var $val} err]} {
        output "epics notification \"caput $epics_var $val\" failed: $err" tag_red
    }
}

# epics_put_ved sets the EPICS variable given in epics_notification(keyword)
#   only if ved is equal to notification_source(keyword)

proc epics_put_ved {ved keyword val unittype} {

    if {![interp eval mastersetup info exists notification_source($keyword)]} {
        return
    }

    set source [interp eval mastersetup set notification_source($keyword)]

    if {![string equal $ved $source]} {
        return
    }

    epics_put $keyword $val $unittype

}
