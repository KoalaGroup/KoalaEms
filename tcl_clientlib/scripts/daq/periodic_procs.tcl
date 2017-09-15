# $ZEL: periodic_procs.tcl,v 1.1 2009/04/01 18:15:30 wuestner Exp $
# copyright:
#   2009 P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_daq(status)
# global_periodic_${type}_procs($ved) with type == rate, size or time
#       size and time are not supported yet
#       other procedures can be created but are never used
#

proc collect_periodic_proc {name space} {
    global global_verbose

    output "check periodic procs for $name" tag_blue

    set proctypes [$space eval array names periodic_procs]
    foreach type $proctypes {
        set procs [$space eval set periodic_procs($type)]
        if {$global_verbose} {
            output "$type procs for $name: $procs"
        }
        global global_periodic_${type}_procs
        foreach proc $procs {
            set globalname periodic_${type}_${name}_$proc
            global $globalname
            catch {unset $globalname}
            if {[catch {array set $globalname [$space eval array get $proc]} mist]} {
                output_append "no configuration data for $proc in $name found" tag_red
                return -1
            }
            set vedname [$space eval ved name]
            if {![info exists ${globalname}(procname)]} {
                output_append "configuration array for $proc in $name has no procname member" tag_red
                return -1
            }
            set ${globalname}(space) $space
            set ${globalname}(vedname) $vedname
            set ${globalname}(setupname) $name
            set ${globalname}(localname) $proc
            set ${globalname}(last_called) 0

            lappend global_periodic_${type}_procs($vedname) $globalname
        }
    }
    return 0
}

proc delete_old_periodic_procs {} {
    # delete old periodic_procs
    set old [info globals global_periodic_*_procs]
    foreach i $old {
        global $i
        unset $i
    }
}

proc collect_periodic_procs {} {
    global global_setup global_setupdata global_verbose

    if {$global_verbose} {
        output "collecting periodic procs"
    }

    delete_old_periodic_procs

    # collect all periodic_procs from mastersetup
    set res [collect_periodic_proc mastersetup mastersetup]
    if {$res} {return $res}

    # collect all periodic_procs from all ved spaces
    foreach i $global_setupdata(names) {
        set res [collect_periodic_proc $i ved_setup_$i]
        if {$res} {return $res}
    }

    if {$global_verbose} {
        foreach type {time rate size} {
            output "dumping for $type"
            global global_periodic_${type}_procs
            set veds [array names global_periodic_${type}_procs]
            output_append "dumping for veds $veds"
            foreach ved $veds {
                set procs [set global_periodic_${type}_procs($ved)]
                foreach proc $procs {
                    global $proc
                    output_append "periodic_${type}_proc $proc in [set ${proc}(setupname)]:"
                    foreach i [array names $proc] {
                        output_append "$i: [set ${proc}($i)]"
                    }
                }
            }
        }
    }
    return 0
}

proc event_rate_handler {ved rate} {
    global global_periodic_rate_procs

    #output "event_rate_handler $ved $rate" tag_green

    set vedname [$ved name]

    #output_append "vedname: $vedname" tag_green

    if {![info exists global_periodic_rate_procs($vedname)]} {
        return
    }

    foreach proc $global_periodic_rate_procs($vedname) {
        #output "found proc $proc" color:#0000a0
        global $proc
        array set arr [array get $proc]
        if {[info exists arr(rate_below)]} {
            if {$rate>=$arr(rate_below)} {
                #output_append "rate is too high, returning" color:red
                return
            }
        }
        if {[info exists arr(rate_above)]} {
            if {$rate<$arr(rate_above)} {
                #output_append "rate is too low, returning" color:red
                return
            }
        }
        set now [clock seconds]
        if {$now<$arr(last_called)+$arr(silent_period)} {
            #output_append "recently called, returning" color:red
            return
        }
        #output_append "try to call it" color:purple
        #output_append "space is $arr(space)" color:red
        set res [$arr(space) eval $arr(procname) $ved $rate $arr(localname)]
        set ${proc}(last_called) $now

        if {$res} {
            output "periodic_proc returned error ==> stopping readout" tag_red
            $ved readout stop
            change_status_fatal
        }

    }

    #void
}

proc file_size_handler {ved idx MBytes} {
    global global_periodic_size_procs

    #output "file_size_handler $ved $idx $MBytes"

    #void
}

# proc start_periodic_procs {} {
#     global global_setup global_setupdata global_verbose
#     global global_periodic_procs
# 
#     output "starting periodic procs"
# 
#     foreach proc $global_periodic_procs {
#         global $proc
#         output "periodic_proc $proc in [set ${proc}(setupname)]:" tag_blue
#         array set arr [array get $proc]
#         if {[info exists arr(scope)] && [string equal $arr(scope), global]} {
#             output "periodic_procs in global scope not implemented yet" tag_red
#             return -1
#         } else {
#             set space $arr(space)
#             #set ${space}::periodic_procname $arr(procname)
#             #set ${space}::periodic_procarr $arr(localname)
#             $space eval $arr(procname) $arr(localname)
#         }
# 
#     after 1000 
# 
# 
#     }
#     return 0
# }

proc init_periodic_procs {} {
    global global_setup global_setupdata global_verbose
    global global_periodic_procs

    set global_periodic_procs {}

    set res [collect_periodic_procs]
    if {$res} {return $res}

#     set res [start_periodic_procs]
#     if {$res} {return $res}

    return 0

}
