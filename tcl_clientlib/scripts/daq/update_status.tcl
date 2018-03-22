# $ZEL: update_status.tcl,v 1.20 2017/11/08 00:17:20 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

# callbacks:

proc got_ro_status {ved args} {
  global global_rostatvar_$ved
  global global_rostatvar_num_$ved
  global global_rostatvar_rate_$ved
  global global_status_loop
  global spacename
  #output "got_ro_status: $ved $args" tag_orange
  #output "got_ro_status: space=$spacename"
  set text $args
  set ev [lindex $args 1]
  set running [string equal [lindex $args 0] "running"]
  if {$running} {
    set old [set global_rostatvar_num_${ved}(ro)]
    set rate [expr ($ev-$old)*1000./$global_status_loop(delay)]
    set global_rostatvar_rate_$ved $rate
    append text " [format {(%.1f ev/s)} $rate]"
    epics_put_ved $ved eventrate $rate rate
  } else {
    epics_put_ved $ved eventrate 0 none
  }
  set global_rostatvar_${ved}(ro) $text
  set global_rostatvar_num_${ved}(ro) $ev  
  epics_put_ved $ved eventnr $ev none

  update idletasks

  if {$running} {
    event_rate_handler $ved $rate
  }
}

proc got_ro_status_error {ved conf} {
  # output "got_ro_status_error: $ved $conf" tag_orange
  output "readout $ved: [$conf print text]" tag_orange
  $conf delete
}

# auto_restart if maximum filesize reached
# and minimum event rate is below threshold
proc check_auto_restart {ved idx MBytes} {
    global global_daq
    global global_interps
    global global_rostatvar_rate_$ved

    # is DAQ running?
    if {![string equal $global_daq(status) "running"]} {
        return
    }
    # is auto_restart already in progress?
    if {$global_daq(autoreset)} {
        return
    }
    # is maximum_filesize set in setup file and is it already reached?
    set space $global_interps($ved)
    if {![$space eval info exists dataout_maximum_filesize($idx)]} {
        return
    }
    set maximum [$space eval set dataout_maximum_filesize($idx)]
    if {$MBytes<$maximum} {
        return
    }

    # is a event rate threshold given and are we below this?
    if {[$space eval info exists rate_threshold_for_restart]} {
        set threshold [$space eval set rate_threshold_for_restart]
	if {[$space eval info exists rate_source_for_restart]} {
		set rved [$space eval set rate_source_for_restart]
		global global_rostatvar_rate_$rved
		set rate [set global_rostatvar_rate_$rved]
	} else {
		set rate [set global_rostatvar_rate_$ved]
	}
        set rate [set global_rostatvar_rate_$ved]
        #output "threshold=$threshold rate=$rate"
        if {$rate>$threshold} {
            if {![info exists global_daq(auto_restart_message)] ||
                !$global_daq(auto_restart_message)} {
                output "maximum filesize reached,\
waiting for event rate to go below $threshold/s"
                set global_daq(auto_restart_message) 1
            }
            return
        }
    }
    # prevent concurrent invovations of automatic_stop
    set global_daq(autoreset) 1
    # automatic_stop will call automatic_restart
    set global_daq(auto_restart_message) 0
    automatic_stop $ved $idx filesize_reached
}

proc got_do_status {ved idx autoreset args} {
    global global_rostatvar_$ved
    global global_rostatvar_num_$ved
    global global_status_loop
    global global_intsize
    global global_interps
    global global_daq

    set key ${ved}_${idx}
    if {[lindex $args 0]==-2} {               ; # version 2
        set err [lindex $args 1]              ; # do error
        set stat [statnames [lindex $args 2]] ; # do status
        set doswitch [lindex $args 3]         ; # do switch
        set clusters_h [lindex $args 5]
        set clusters_l [lindex $args 6]
        set clusters $clusters_l
        set words_h [lindex $args 7]
        set words_l [lindex $args 8]
        if {$global_intsize<64} {
            set words $words_l ;# words_h is ignored for datarate
            if {$words_l<0} { ;# [expr $words*4/1048576]
                set MBytes [expr (($words_l&0x7fffffff)>>18)|0x2000]
            } else {
                set MBytes [expr $words_l>>18]  
            }
            set MBytes [expr $MBytes|(($words_h<<14)&0x7fffffff)]
        } else {
            set words [expr $words_l+($words_h<<32)]
            set MBytes [expr $words/262144] ;# [expr $words*4/1048576]  
        }
        set tstatus [lrange $args 16 end]
    } else {                                  ; # version 1
        set err [lindex $args 0]              ; # do error
        set stat [statnames [lindex $args 1]] ; # do status
        set doswitch [lindex $args 2]         ; # do switch
        set clusters [lindex $args 4]
        set words [lindex $args 5]

        if {$words<0} { ;# [expr $words*4/1048576]
            set MBytes [expr (($words&0x7fffffff)>>18)|0x2000]
        } else {
            set MBytes [expr $words>>18]  
        }
        set tstatus [lrange $args 11 end]
    }
    set text $stat
    if {$doswitch==1} {append text " (disabled)"}
    if {$err} {append text " (error $err)"}
    #append text " [format {%d} $clusters]"
    set running [string equal $stat "running"]
    set flushing [string equal $stat "flushing"]
    if {$running || $flushing} {
        set old [set global_rostatvar_num_${ved}($idx)]
        set rate [expr 1000.*($words-$old)/$global_status_loop(delay)]
# if {[string equal $ved ved_ebj2]} {
# output "rate: ved=$ved words=$words old=$old delay=$global_status_loop(delay) rate=$rate" tag_red
# }
        # [expr ($words-$old)*4000./$global_status_loop(delay)/1048576.0]
        set MBrate [expr ($words-$old)/(262.144*$global_status_loop(delay))]
# output "MBrate ved=$ved $MBrate" tag_red
        append text " [format {(%.3f MB/s; %d MB)} $MBrate $MBytes]"
    }
    set global_rostatvar_${ved}($idx) $text
    set global_rostatvar_num_${ved}($idx) $words
    global global_dataout_$key
    if [set global_dataout_${key}(istape)] {
        if {$global_intsize<64} {
            update_tstatus_32 $key $tstatus
        } else {
            update_tstatus_64 $key $tstatus
        }
    }

    if {$running} {
        file_size_handler $ved $idx $MBytes
        epics_put_ved $ved filesize [expr $words*4] size
        epics_put_ved $ved datarate [expr $rate*4] rate
    } else {
        epics_put_ved $ved datarate 0 none
    }

    if {!$autoreset} {
        check_auto_restart $ved $idx $MBytes
    }
}

proc got_do_status_error {ved do conf} {
  # output "got_do_status_error: $ved $do $conf" tag_orange
  output "$ved dataout $do: [$conf print text]" tag_orange
  $conf delete
}

proc status_update_do {ved idx} {
  global global_daq
  #output "status_update_do $ved $idx"
  set confmode [$ved confmode asynchron]
  if [catch {
    $ved dataout status $idx 2 : got_do_status $ved $idx $global_daq(autoreset) ? got_do_status_error $ved $idx
    } mist] {
    output "status_update_do $ved do $idx: $mist" tag_blue
  }
  $ved confmode $confmode
}

proc status_update_ved {ved} {
  global global_dataoutlist

  #output "status_update_ved $ved"
  set confmode [$ved confmode asynchron]
  if [catch {
    $ved readout status : got_ro_status $ved ? got_ro_status_error $ved
  } mist ] {
    output "status_update_ved $ved: $mist" tag_blue
  } else {
    foreach idx $global_dataoutlist($ved) {
      status_update_do $ved $idx
    }
  }
  $ved confmode $confmode
}

proc status_update {} {
  global global_veds
  #output "status_update" tag_blue
  foreach i [array names global_veds] {
    status_update_ved $global_veds($i)
  }
}

proc status_loop {} {
  global global_status_loop
  #output "status_loop"
  status_update
  set global_status_loop(id) [after $global_status_loop(delay) status_loop]
}

proc start_status_loop {} {
  global global_status_loop

  if [info exists global_status_loop(id)] {
    after cancel $global_status_loop(id)
  }
  set global_status_loop(delay) 5000
  #set global_status_loop(id) [after $global_status_loop(delay) status_loop]
  set global_status_loop(id) [after 100 status_loop]
  #output "start_status_loop"
}

proc stop_status_loop {} {
  global global_status_loop

  if [info exists global_status_loop(id)] {
    after cancel $global_status_loop(id)
    unset global_status_loop(id)
  }
}
