# $ZEL: calculate_tapeend.tcl,v 1.4 2007/04/15 13:15:09 wuestner Exp $
# copyright 2003
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc calculate_tapeend {key ft rest jetzt} {
        # time of start of measurement (to avoid too large timestamps)
    global t0 clockformat
        # list of the last remaining blocks (size 4096)
    upvar #0 tapeend_reste($key) reste
        # list of the last times when remaining blocks have been reported
    upvar #0 tapeend_zeiten($key) zeiten
        # last 'null' (0/1)
        # 'null' is 1 if the datarate is lower than half of the average
    upvar #0 tapeend_last_null($key) last_null
        # incremented at each invocation of calc_end; 
    upvar #0 tapeend_last_gap($key) last_gap
        # list of the last gaps between 'flat tops'
        # zeroed if null changes from 0 to 1
    upvar #0 tapeend_gaps($key) gaps
        # distance of 5(?) gaps
    upvar #0 tapeend_gapsum($key) gapsum
        # number of gaps seen so far (only gaps_found!=0 is used)
    upvar #0 tapeend_gaps_found($key) gaps_found
        # used to calculate datarate between invocations
    upvar #0 tapeend_old_ft($key) old_ft
    upvar #0 tapeend_old_time($key) old_time

    lappend reste [expr double($rest)]
    lappend zeiten $jetzt ;# jetzt is already double

    set max 120 ;# limit calculations to 10 minutes
    
    if {($gaps_found>2) && ($gapsum>10)} {set max $gapsum}
    while {[llength $reste]>$max} { ;# truncate the lists
        set reste [lrange $reste 1 end]
        set zeiten [lrange $zeiten 1 end]
    }
    set n [llength $reste]

    if {$old_time} {
        ;# rate of data transferred to tape (after compression)
        set trate [expr ($ft-$old_ft)/($jetzt-$old_time)]
    } else {
        set trate 0
    }
    set t4rate [expr $trate/4096] ;# the same in terms of 4 KBye
    set old_ft $ft
    set old_time $jetzt

    set xbang 0
    if {$n>1} {
        set xqsum 0; set xsum 0; set ysum 0; set xysum 0
        for {set i [expr $n-1]} {$i>=0} {incr i -1} {
            set x [lindex $zeiten $i]
            set y [lindex $reste $i]
            set xqsum [expr $xqsum+$x*$x]
            set xsum [expr $xsum+$x]
            set ysum [expr $ysum+$y]
            set xysum [expr $xysum+$x*$y]
        }

        set q [expr $n*$xqsum-$xsum*$xsum]
        set b [expr ($n*$xysum-$xsum*$ysum)/$q];
        set a [expr ($ysum-$b*$xsum)/$n];

        set null [expr $t4rate<$b*-.5]
        set xrest [expr $a+$b*$jetzt]
        set xrd [expr $rest/$xrest-1.]
        set bang [expr -$a/$b]
        set xbang [expr int($bang+$t0)]
        set sbang [clock format $xbang -format $clockformat]

        #puts -nonewline [format {n=%3d r=%d b=%+6.1f bang=%6.0f %+f %s %6.0f %5.1f %d %d} \
        #    $n $rest $b $bang $xrd $sbang $trate $t4rate $null $last_gap]
        #puts -nonewline "n=$n r=$rest b=$b bang=$bang $xrd $sbang $trate $t4rate $null $last_gap"
        if {!$null && $last_null} {
            if {$gaps_found>1} {
                lappend gaps $last_gap

                set gapsum 0

                set gapnum [llength $gaps]
                if {$gapnum>10} {set gaps [lrange $gaps 1 end]}
                set gapsum 0
                foreach g $gaps {incr gapsum $g}
                foreach g $gaps {puts -nonewline " $g"}
            }
            set last_gap 0
            incr gaps_found
        }
        #puts ""
        incr last_gap
        set last_null $null
    } else {
        #puts [format {n=%3d r=%d} $n $rest]
        #puts "n=$n r=$rest"
    }
    return $xbang
}

proc init_tapeend {key} {
    global tapeend_last_null
    global tapeend_last_gap
    global tapeend_gaps
    global tapeend_gapsum
    global tapeend_gaps_found
    global tapeend_old_ft
    global tapeend_old_time

    set tapeend_last_null($key) 0
    set tapeend_last_gap($key) 0
    set tapeend_gaps($key) {}
    set tapeend_gapsum($key) 0
    set tapeend_gaps_found($key) 0
    set tapeend_old_ft($key) 0
    set tapeend_old_time($key) 0
}
