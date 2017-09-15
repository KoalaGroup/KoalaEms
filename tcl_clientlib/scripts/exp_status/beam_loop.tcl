proc smooth_bct {val} {
  global bct_arr bct_idx bct_max

  if {$bct_arr(0)<0} {
    for {set i 0} {$i<$bct_max } {incr i} {set bct_arr($i) $val}
  } else {
    set bct_arr($bct_idx) $val
  }
  incr bct_idx
  if {$bct_idx==$bct_max} {set bct_idx 0}

  set bct_l {} ; set n $bct_max
  for {set i 0} {$i<$bct_max} {incr i} {lappend bct_l $bct_arr($i)}
  #puts "smooth_bct: $bct_l"

  set sum 0 ; set len $bct_max
  for {set i 0} {$i<$len} {incr i} {set sum [expr $sum+[lindex $bct_l $i]]}
  set med [expr double($sum)/$len]
  #puts "med=$med"
  for {set c 0} {$c<2} {incr c} {
    set idx 0
    set dif0 [expr abs([lindex $bct_l 0]-$med)]
    for {set i 1} {$i<$len} {incr i} {
      set dif1 [expr abs([lindex $bct_l $i]-$med)]
      if {$dif1>$dif0} {
        set idx $i
        set dif0 $dif1
      }
    }
    #puts "  exclude ($idx): [lindex $bct_l $idx]"
    set bct_l [lreplace $bct_l $idx $idx]
    #puts "  new list: $bct_l"
    set sum 0 ; set len [llength $bct_l]
    for {set i 0} {$i<$len} {incr i} {set sum [expr $sum+[lindex $bct_l $i]]}
    set med [expr double($sum)/$len]
    #puts "new med=$med len=$len"
  }
  return $med
}

proc smooth_targ {val} {
  global targ_arr targ_idx targ_max

  if {$targ_arr(0)<0} {
    for {set i 0} {$i<$targ_max } {incr i} {set targ_arr($i) $val}
  } else {
    set targ_arr($targ_idx) $val
  }
  incr targ_idx
  if {$targ_idx==$targ_max} {set targ_idx 0}

  set targ_l {} ; set n $targ_max
  for {set i 0} {$i<$targ_max} {incr i} {lappend targ_l $targ_arr($i)}

  set sum 0 ; set len $targ_max
  for {set i 0} {$i<$len} {incr i} {set sum [expr $sum+[lindex $targ_l $i]]}
  set med [expr double($sum)/$len]
  for {set c 0} {$c<2} {incr c} {
    set idx 0
    set dif0 [expr abs([lindex $targ_l 0]-$med)]
    for {set i 1} {$i<$len} {incr i} {
      set dif1 [expr abs([lindex $targ_l $i]-$med)]
      if {$dif1>$dif0} {
        set idx $i
        set dif0 $dif1
      }
    }
    set targ_l [lreplace $targ_l $idx $idx]
    set sum 0 ; set len [llength $targ_l]
    for {set i 0} {$i<$len} {incr i} {set sum [expr $sum+[lindex $targ_l $i]]}
    set med [expr double($sum)/$len]
  }
  return $med
}

proc beam_data {path} {
  global global_beam_data

  set bl [read $path 4]
  set n [binary scan $bl I l]
  if {$n!=1} {
    puts "beam_data(a): n=$n"
  }
  set bl [read $path [expr 4*$l]]
  set n [binary scan $bl IIIII sec usec rl bct targ]
  if {$n!=5} {
    puts "beam_data(b): n=$n"
  }
  if {$rl!=8} {
    puts "beam_data: rl=$rl"
  }
  puts "   bct=$bct"
  set bct [smooth_bct [expr $bct/2764.8]]
  #set targ [expr $targ/2764.8]
  set targ [smooth_targ [expr $targ/2764.8]]

  set global_beam_data(valid) 1
  set global_beam_data(time) $sec
  set global_beam_data(bct) $bct
  set global_beam_data(targ) $targ

  #puts "beam time: [clock format $sec]"
  puts [format {bct=%.3f   targ=%.3f} $bct $targ]
  puts ""
}

proc beam_go {} {
  global global_beam
  global bct_arr bct_idx bct_max
  global targ_arr targ_idx targ_max

  set bct_max 10
  #for {set i 0} {$i<$bct_max } {incr i} {set bct_arr($i) 0}
  set bct_arr(0) -1
  set bct_idx 0

  set targ_max 10
  set targ_arr(0) -1
  set targ_idx 0

  set global_beam(path) [socket $global_beam(host) $global_beam(port)]
  set bl [binary format c 10]
  puts -nonewline $global_beam(path) $bl
  flush $global_beam(path)
  fileevent $global_beam(path) readable "beam_data $global_beam(path)"
}

proc start_beam_loop {} {
  after 1000 beam_go
}
