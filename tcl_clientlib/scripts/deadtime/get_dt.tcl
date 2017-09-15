proc get_dt {ved master} {
  global last_time
  global last_event
  global last_rejectcount

  set ldtma 0

  if {$master} {
    set l [$ved command {Timestamp 1 GetSyncStatist {3 3 -1 0}}]
  } else {
    set l [$ved command {Timestamp 1 GetSyncStatist {1 1 -1 0}}]
  }
  set sec [lindex $l 0]
  set usec [lindex $l 1]
  set time [expr double($sec)+double($usec)/1000000.]
#puts $l
  set event [lindex $l 3]
  if {$master} {
    set rejectcount [lindex $l 5]
    set hrejectcount [lindex $l 4]
    if {$hrejectcount!=0} {puts "hrejectcount=$hrejectcount"}
  }
  set num [lindex $l 6]
  set rl [lrange $l 7 end]
#puts "num =$num"
  for {set i 0} {$i<$num} {incr i} {
    #puts "i=$i list=$rl"
    set typ [lindex $rl 0]
#puts "typ =$typ"
    set summe($typ) [lindex $rl 6]
    set hsumme($typ) [lindex $rl 5]
    #if {$hsumme($typ)!=0} {puts "hsumme($typ)=$hsumme($typ)"}

    set rl [lrange $rl 12 end]
  }

  if {$last_event($ved)>0} {
    set measured [expr $event-$last_event($ved)]
    set tdiff [expr $time-$last_time($ved)]
    if {$master} {
      set rejected [expr $rejectcount-$last_rejectcount($ved)]
      set triggered [expr $measured+$rejected]
      #puts "tdiff=$tdiff"
      puts -nonewline [format {measured: %1f/s triggered: %1f/s} [expr $measured/$tdiff] [expr $triggered/$tdiff]]
      set tdeadtime [expr double($summe(2))/10000000.] ;# in s
      # gemessene totzeit in %
      set tdtmr [expr $tdeadtime/$tdiff]
        puts {}
        puts "deadtime/time     =[format {%3f} $tdtmr]"
        if {$triggered} {
            set w_dead [expr double($rejected)/double($triggered)]
            set w_alive [expr double($measured)/double($triggered)]
            puts "measured/triggered=[format {%3f} $w_alive]"
            puts "rejected/triggered =[format {%3f} $w_dead]"
            puts "dead+alive=[format {%3f} [expr $w_alive+$w_dead]]"
            puts "tdiff=$tdiff tdeadtime=$tdeadtime"
        }
      puts -nonewline [format { tdt(m): %3f%%} [expr $tdtmr*100.]]
      if {$measured>0} {
        # gemessene totzeit pro event in ms
        set tdtma [expr $tdeadtime/$measured*1000.]
        puts -nonewline [format { %3f ms} $tdtma]
      }

      if {$triggered>0} {
        set dt_rel [expr double($rejected)/double($triggered)]
        puts -nonewline [format { tdt(c): %3f%%} [expr $dt_rel*100.]]
      }
      puts ""
    }
    if {$measured>0} {
      set ldeadtime [expr double($summe(1))/10000000.]
      # gemessene totzeit pro event in ms
      set ldtma [expr $ldeadtime/$measured*1000.]
    }
    
  }

  set last_time($ved) $time
  set last_event($ved) $event
  if {$master} {
    set last_rejectcount($ved) $rejectcount
  }
  return $ldtma
}
