proc v792_status1 {stat1} {
    #puts {status1:}
    puts -nonewline {  }
    if {$stat1 & 0x100} {
        puts -nonewline {EVRDY }
    }
    if {$stat1 & 0x80} {
        puts -nonewline {TERM_OFF }
    }
    if {$stat1 & 0x40} {
        puts -nonewline {TERM_ON }
    }
    if {$stat1 & 0x20} {
        puts -nonewline {PURGED }
    }
    if {$stat1 & 0x10} {
        puts -nonewline {AMNESIA }
    }
    if {$stat1 & 0x8} {
        puts -nonewline {GLOBAL_BUSY }
    }
    if {$stat1 & 0x4} {
        puts -nonewline {BUSY }
    }
    if {$stat1 & 0x2} {
        puts -nonewline {GLOBAL_DREADY }
    }
    if {$stat1 & 0x1} {
        puts -nonewline {DREADY }
    }
    puts {}
}
proc v792_status2 {stat2} {
    #puts {status2:}
    puts "  piggy_back: [expr ($stat2>>4)&0xf] "
    puts -nonewline {  }
    if {$stat2 & 0x4} {
        puts -nonewline {BUFFER_FULL }
    }
    if {$stat2 & 0x2} {
        puts -nonewline {BUFFER_EMPTY }
    }
    puts {}
}
proc v792_control1 {ctrl1} {
    puts -nonewline {  }
    if {$ctrl1 & 0x40} {
        puts -nonewline {ALIGN64 }
    }
    if {$ctrl1 & 0x20} {
        puts -nonewline {BERR_ENABLE }
    }
    if {$ctrl1 & 0x10} {
        puts -nonewline {PROG_RESET }
    }
    if {$ctrl1 & 0x4} {
        puts -nonewline {BLKEND }
    }
    puts {}
}
proc v792_bit2 {bit2} {
    puts -nonewline {  }
    if {$bit2 & 0x4000} {
        puts -nonewline {ALL_TRG }
    }
    if {$bit2 & 0x2000} {
        puts -nonewline {SLIDE_SUB_ENA }
    }
    if {$bit2 & 0x1000} {
        puts -nonewline {EMPTY_ENABLE }
    }
    if {$bit2 & 0x800} {
        puts -nonewline {AUTO_INCR }
    }
    if {$bit2 & 0x100} {
        puts -nonewline {STEP_TH }
    }
    if {$bit2 & 0x80} {
        puts -nonewline {SLIDE_ENA }
    }
    if {$bit2 & 0x40} {
        puts -nonewline {TEST_ACQ }
    }
    if {$bit2 & 0x10} {
        puts -nonewline {LOW_THRESH_ENA }
    }
    if {$bit2 & 0x8} {
        puts -nonewline {OVER_RANGE_ENA }
    }
    if {$bit2 & 0x4} {
        puts -nonewline {CLEAR_DATA }
    }
    if {$bit2 & 0x2} {
        puts -nonewline {OFFLINE }
    }
    if {$bit2 & 0x1} {
        puts -nonewline {TEST_MEM }
    }
    puts {}
}
proc v792stat {is member} {
    set firmware  [$is command1 VMEread2 $member 0x1000]
    puts [format {firmware : 0x%4x} $firmware]
    set geo       [$is command1 VMEread2 $member 0x1002]
    puts [format {geo      : 0x%4x} [expr $geo & 0x1f]]
    set mcastaddr [$is command1 VMEread2 $member 0x1004]
    puts [format {mcastaddr: 0x%4x} $mcastaddr]
    set bit1      [$is command1 VMEread2 $member 0x1006]
    puts [format {bit1     : 0x%4x} $bit1]
    set stat1     [$is command1 VMEread2 $member 0x100E]
    puts [format {status1  : 0x%4x} $stat1]
    v792_status1 $stat1
    set ctrl1     [$is command1 VMEread2 $member 0x1010]
    puts [format {control1 : 0x%4x} $ctrl1]
    v792_control1 $ctrl1
    set mcastctrl [$is command1 VMEread2 $member 0x101A]
    puts [format {mcastctrl: 0x%4x} [expr $mcastctrl & 0x3]]
    set stat2     [$is command1 VMEread2 $member 0x1022]
    puts [format {status2  : 0x%4x} $stat2]
    v792_status2 $stat2
    set evnt_low  [$is command1 VMEread2 $member 0x1024]
    puts [format {evnt_low : 0x%4x} $evnt_low]
    set evnt_high [$is command1 VMEread2 $member 0x1026]
    puts [format {evnt_high: 0x%4x} $evnt_high]
    set fcw       [$is command1 VMEread2 $member 0x102E]
    puts [format {fcw      : 0x%4x} $fcw]
    set bit2      [$is command1 VMEread2 $member 0x1032]
    puts [format {bit2     : 0x%4x} $bit2]
    v792_bit2 $bit2
    set Iped      [$is command1 VMEread2 $member 0x1060]
    puts [format {Iped     : 0x%4x} $Iped]
}

proc v792stat_geo {is member} {
    set firmware  [$is command1 VMEread $member 0x1000 0x2f 2 1]
    puts [format {firmware : 0x%4x} [expr $firmware&0xffff]]
    set geo       [$is command1 VMEread $member 0x1002 0x2f 2 1]
    puts [format {geo      : 0x%4x} [expr $geo & 0x1f]]
    set mcastaddr [$is command1 VMEread $member 0x1004 0x2f 2 1]
    puts [format {mcastaddr: 0x%4x} [expr $mcastaddr&0xff]]
    set bit1      [$is command1 VMEread $member 0x1006 0x2f 2 1]
    puts [format {bit1     : 0x%4x} [expr $bit1&0x98]]
    set stat1     [$is command1 VMEread $member 0x100E 0x2f 2 1]
    puts [format {status1  : 0x%4x} [expr $stat1&0x1ff]]
    v792_status1 $stat1
    set ctrl1     [$is command1 VMEread $member 0x1010 0x2f 2 1]
    puts [format {control1 : 0x%4x} [expr $ctrl1&0x74]]
    v792_control1 $ctrl1
    set adrhigh   [$is command1 VMEread $member 0x1012 0x2f 2 1]
    puts [format {adrhigh  : 0x%4x} [expr $adrhigh&0xff]]
    set adrlow    [$is command1 VMEread $member 0x1014 0x2f 2 1]
    puts [format {adrlow   : 0x%4x} [expr $adrlow&0xff]]
    set mcastctrl [$is command1 VMEread $member 0x101A 0x2f 2 1]
    puts [format {mcastctrl: 0x%4x} [expr $mcastctrl & 0x3]]
    set stat2     [$is command1 VMEread $member 0x1022 0x2f 2 1]
    puts [format {status2  : 0x%4x} [expr $stat2&0xf6]]
    v792_status2 $stat2
    set evnt_low  [$is command1 VMEread $member 0x1024 0x2f 2 1]
    puts [format {evnt_low : 0x%4x} [expr $evnt_low&0xffff]]
    set evnt_high [$is command1 VMEread $member 0x1026 0x2f 2 1]
    puts [format {evnt_high: 0x%4x} [expr $evnt_high&0xff]]
    set fcw       [$is command1 VMEread $member 0x102E 0x2f 2 1]
    puts [format {fcw      : 0x%4x} [expr $fcw&0x3ff]]
    set bit2      [$is command1 VMEread $member 0x1032 0x2f 2 1]
    puts [format {bit2     : 0x%4x} [expr $bit2&0x7fff]]
    v792_bit2 $bit2
    set Iped      [$is command1 VMEread $member 0x1060 0x2f 2 1]
    puts [format {Iped     : 0x%4x} [expr $Iped&0xff]]
}

proc v792stat_thresh {is member} {
    set i 0
    for {set a 0x1080} {$a<0x10c0} {incr a 2} {
        set val [expr [is_t05_3 command1 VMEread $member $a 0x2f 2 1] & 0xff]
        puts [format {%2d: %02x} $i $val]
        incr i
    }
}

proc v792_decode {word} {
    set geo [expr ($word>>27)&0x1f]
    set code [expr ($word>>24)&0x7]
    switch $code {
        0 {
            set channel [expr ($word>>16)&0x1f]
            set un [expr ($word>>13)&1]
            set ov [expr ($word>>12)&1]
            set val [expr $word&0xfff]
            puts -nonewline "($geo:$channel) $val"
            if {$un} {puts -nonewline U}
            if {$ov} {puts -nonewline O}
            puts -nonewline { }
        }
        2 {
            set crate [expr ($word>>16)&0xff]
            set count [expr ($word>>8)&0x3f]
            puts "geo $geo crate $crate count $count"
        }
        4 {
            set counter [expr $word&0xffffff]
            puts {}
            puts "eventcount $counter"
        }
        6 {
            puts -nonewline "invalid "
        }
    }
}

proc v792_decode2 {words} {
    set invalids 0
    array unset vals
    array unset os
    array unset counts
    set geos {}

    foreach word $words {
        set geo [expr ($word>>27)&0x1f]
        set code [expr ($word>>24)&0x7]
        switch $code {
            0 {
                set channel [expr ($word>>16)&0x1f]
                set idx $geo:$channel
                set vals($idx) [expr $word&0xfff]
                set un         [expr ($word>>13)&1]
                set ov         [expr ($word>>12)&1]
                if {$un && $ov} {
                    puts "invalid un/ov $idx"                }
                set O { }
                if {$un} {set O U}
                if {$ov} {set O O}
                set os($idx) $O
            }
            2 {
                lappend geos $geo
                set counts($geo) [expr ($word>>8)&0x3f]
            }
            4 {
                set eventcount($geo) [expr $word&0xffffff]
            }
            6 {
                incr invalids
            }
        }
    }

    set geos [lsort -integer $geos]
    puts -nonewline {       }
    foreach g $geos {
        puts -nonewline "$g $counts($g)      "
    }
    puts {}
    for {set i 0} {$i<32} {incr i} {
        puts -nonewline [format {%2d} $i]
        foreach g $geos {
            set idx $g:$i
            if [info exists vals($idx)] {
                puts -nonewline [format { %8d%s} $vals($idx) $os($idx)]
            } else {
                puts -nonewline { -------- }
            }
        }
        puts {}
    }
    puts "invalids: $invalids"
}

proc v792_read {is addr} {
    set data [$is command1 v792cblt $addr]
    puts "num [lindex $data 0]"
    set data [lrange $data 1 end]
    v792_decode2 $data
}
