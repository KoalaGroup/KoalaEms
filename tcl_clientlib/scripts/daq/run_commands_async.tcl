# $ZEL: run_commands_async.tcl,v 1.5 2009/08/19 17:01:40 wuestner Exp $
# copyright 2008
#   Peter Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global run_commands_arr
#   run_commands_arr is an array with one element for each proclist or
#   procedure for each VED
#   the index is $space.${t}_proclist or similar
#   the elements are lists:
#     name proclist-name
#   run_commands_arr is filled during calling of the commands and elements
#   are removed when a command succeeds (or failes)
#   we have to wait until the array is empty
# global run_commands_res
#   run_commands_res will be set to -1 if at least one of the commands failes

proc runcommands_success {seqno args} {
    global global_verbose
    global run_commands_arr
    global run_commands

    if {![info exists run_commands_arr($seqno)]} {
        output "runcommands_success: unexpected response $seqno" tag_blue
        set run_commands(res) -1
    } else {
        if {$global_verbose} {
            set l $run_commands_arr($seqno)
            set space ved_setup_[lindex $l 0]
            set vedname [$space eval ved name]
            if {[string equal [lindex $l 0] $vedname]} {
                set xname $vedname
            } else {
                set xname "[lindex $l 0] ($vedname)"
            }
            output "[lindex $l 1] for $xname successfull" tag_green
            output "args: $args"
        }
        unset run_commands_arr($seqno)
    }
}

proc runcommands_error {seqno args} {
    global run_commands_arr
    global run_commands

    if {![info exists run_commands_arr($seqno)]} {
        output "runcommands_error: unexpected response $seqno" tag_blue
    } else {
        set l $run_commands_arr($seqno)
        set space ved_setup_[lindex $l 0]
        set vedname [$space eval ved name]
        if {[string equal [lindex $l 0] $vedname]} {
            set xname $vedname
        } else {
            set xname "[lindex $l 0] ($vedname)"
        }
        output "[lindex $l 1] for $xname unsuccessfull" tag_red
        set conf [lindex $args 0]
        decode_error_confirmation [lindex $l 0] $conf [lindex $l 2]
        $conf delete
        unset run_commands_arr($seqno)
    }
    set run_commands(res) -1
}

proc call_proclist_for_ved {name listname} {
    global global_verbose
    global run_commands_arr
    global run_commands
    set space ved_setup_$name
    set res 0

    set vedname [$space eval ved name]

    output "executing $listname for $name ($vedname)" tag_green

    set idxs [$space eval array names $listname]
    foreach i [lsort -dictionary $idxs] {
        set lname $listname\($i\)
        if {$global_verbose} {
            output_append "proclist for IS $i: [$space eval set $lname]" tag_green
        }
        set iix [lindex [split $i .] 0]
        if {$iix==0} {
            set is_command ved
        } else {
            set isvar is\($iix\)
            #output_append "isvar is $isvar"
            if {![$space eval info exists $isvar]} {
                output_append "there is no IS $iix configured" tag_red
                return -1
            }
            set is_command [$space eval set $isvar]
        }
        #output_append "is_command is $is_command"
        set confmode [$space eval ved confmode asynchron]
        # we need a 'catch' here to be sure to reset confmode
        if {[catch {$space eval $is_command command \[set $lname\] \
            : runcommands_success $run_commands(seqno) \
            ? runcommands_error $run_commands(seqno)} mist]} {
            output "error: $mist" tag_blue
            set res -1
        }
        $space eval ved confmode $confmode
        if {$res} {
            return -1
        }
        set run_commands_arr($run_commands(seqno)) "$name $lname [list [$space eval set $lname]]"
        incr run_commands(seqno)
    }
    return 0
}

proc call_command_for_ved {name listname} {
    global global_verbose
    set space ved_setup_$name
    set res 0

    set vedname [$space eval ved name]

    output "executing $listname for $name ($vedname)" tag_green

    set idxs [$space eval array names $listname]
    foreach i [lsort -dictionary $idxs] {
        set lname $listname\($i\)
        if {$global_verbose} {
            output_append "command for IS $i: [$space eval set $lname]" tag_green
        }
        set iix [lindex [split $i .] 0]
        if {$iix==0} {
            set is_command nix
        } else {
            set isvar is\($iix\)
            #output_append "isvar is $isvar"
            if {![$space eval info exists $isvar]} {
                output_append "there is no IS $iix configured" tag_red
                return -1
            }
            set is_command [$space eval set $isvar]
        }
        #output_append "is_command is $is_command"
        regsub _command $listname _args argname
        set aname $argname\($i\)
        if {[$space eval info exists $aname]} {
            set args [$space eval set $aname]
            #output_append "args: $args"
        } else {
            #output_append "(no args given)"
        }
        #output "lname=$lname aname=$aname"
        #output "*lname=[$space eval set $lname]"
        #output "*aname=[$space eval set $aname]"

        if {[catch {set res [$space eval eval \[set $lname\] ved $is_command \[set $aname\]]} mist]} {
            output "error: $mist" tag_red
            set res -1
        }

        if {$res} {
            return -1
        }
    }
    return 0
}

proc call_run_commands_async {names t} {
    global global_verbose
    global run_commands_arr
    global run_commands

    # start with an empty array for the outstanding responses
    catch {array unset run_commands_arr}
    array set run_commands_arr {}
    set run_commands(res) 0
    set run_commands(seqno) 0

    output "executing ${t}-commands for $names" tag_green

    foreach name $names {
        if {[ved_setup_$name eval array exists ${t}_proclist]} {
            if {[call_proclist_for_ved $name ${t}_proclist]} {
                return -1
            }
        }
    }

    foreach name $names {
        if {[ved_setup_$name eval array exists ${t}_command]} {
            if {[call_command_for_ved $name ${t}_command]} {
                return -1
            }
        }
    }

    foreach name $names {
        if {[ved_setup_$name eval array exists ${t}_proclist_t]} {
            if {[call_proclist_for_ved $name ${t}_proclist_t]} {
                return -1
            }
        }
    }

    while {[array size run_commands_arr]} {
        output_append "waiting for completion of commands: [array names run_commands_arr]" tag_blue
        if {[array size run_commands_arr]} {
            vwait run_commands_arr
        }
    }
    output_append "${t}-commands executed"
    
    if {$run_commands(res)} {
        output_append "not all ${t}-commands succeeded" tag_red
    }
    return $run_commands(res)
}
