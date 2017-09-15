proc got_ro_status {ved args} {
# z.B. running 31854062 880448267.630741 {1 {0 0}} {4 {}}
# oder running 36454067 880458298.00219095 {1 {0 0}} {4 {5 0}}
# oder inactive 0 880469501.99050105 {1 {}} {2 {2 0 7 0 0 0 0 0 0 0 3 7 0 0 0 0 0 0 0}} {4 {}}

  set namespace ::ns_${ved}

  set key_1 0
  set key_2 0
  set key_4 0

  set time [lindex $args 2]
  set oldtime [set ${namespace}::ro(time)]
  set ${namespace}::ro(time) $time
  set tdiff [expr ($time-$oldtime)]
  set ${namespace}::ro(status) [lindex $args 0]
  set events [lindex $args 1]

  set llist [lrange $args 3 end]
  foreach list $llist {
    set key [lindex $list 0]
    set slist [lindex $list 1]
    switch $key {
      1 {
        # readout data
        if {[llength $slist]==2} {
          set susp [lindex $slist 0]
          set key_1 1
        }
      }
      2 {
        #
        set num [lindex $slist 0]
        set slist [lrange $slist 1 end]
        for {} {$num>0} {incr num -1} {
          set i [lindex $slist 0]
          set anz [lindex $slist 1]
          if {($anz==7) && ([lsearch -exact [set ${namespace}::dataindom] $i]>=0)} {
            switch -- [lindex $slist 2] {
              -3 {set ${namespace}::di_${i}(status) "Error"}
              -2 {set ${namespace}::di_${i}(status) "All done"}
              -1 {set ${namespace}::di_${i}(status) "Stopped"}
              0 {set ${namespace}::di_${i}(status) "Not active"}
              1 {set ${namespace}::di_${i}(status) "Active"}
              default {set ${namespace}::di_${i}(status) [lindex $slist 2]}
            }
            set susp [lindex $slist 5]
            set clust [lindex $slist 6]
            # set events [lindex $slist 7]
            set kbytes [expr [lindex $slist 8]*4]
            if {[set ${namespace}::di_${i}(ok)] && ($tdiff>0)} {
              set ${namespace}::di_${i}(rsusp) [format {%.1f /s} [expr ($susp-[set ${namespace}::di_${i}(susp)])/$tdiff]]
              set ${namespace}::di_${i}(rclust) [format {%.1f /s} [expr ($clust-[set ${namespace}::di_${i}(clust)])/$tdiff]]
              # set ${namespace}::di_${i}(revents) [format {%.1f /s} [expr ($events-[set ${namespace}::di_${i}(events)])/$tdiff]]
              set ${namespace}::di_${i}(rkbytes) [format {%.1f /s} [expr ($kbytes-[set ${namespace}::di_${i}(kbytes)])/$tdiff]]
            }
            set ${namespace}::di_${i}(susp) $susp
            set ${namespace}::di_${i}(clust) $clust
            # set ${namespace}::di_${i}(events) $events
            set ${namespace}::di_${i}(kbytes) $kbytes
            set ${namespace}::di_${i}(ok) 1
          } else {
            puts "datain $i ignored"
          }
          set slist [lrange $slist [expr $anz+2] end]
        }
        set key_2 1
      }
      4 {
        # buffer data
        if {[llength $slist]==2} {
          set ${namespace}::ro(evsize) [lindex $slist 0]
          set copies [lindex $slist 1]
          set key_4 1
        }
      }
      default {
        puts "ved [$ved name]: unknown key $key in ro_status"
      }
    }
  }
  if {($oldtime>0) && ($tdiff>0)} {
    set ${namespace}::ro(revents) \
        [format {%.1f /s} [expr ($events-[set ${namespace}::ro(events)])/$tdiff]]
    if {$key_1} {
      set ${namespace}::ro(rsusp) \
        [format {%.1f /s} [expr ($susp-[set ${namespace}::ro(susp)])/$tdiff]]
    }
    if {$key_4} {
      set ${namespace}::ro(rcopies) \
        [format {%.1f /s} [expr ($copies-[set ${namespace}::ro(copies)])/$tdiff]]
    }
  }
  set ${namespace}::ro(events) $events
  if {$key_1} {
    set ${namespace}::ro(susp) $susp
  }
  if {$key_4} {
    set ${namespace}::ro(copies) $copies
  }
}

proc got_do_status {ved i args} {
# z.B. 0 1 0 0x4 0x0 0x0 0x0 0x0 0x0
# error status switch #(7) clusters words events suspensions clust max_clust 0
# 0     1      2      3    4        5     6      7           8     9
  puts "args: $ved $i $args"
  set confmode [$ved confmode synchron]
  set stat [$ved dataout status $i 1]
  puts "status: $stat"
  set args $stat
  $ved confmode $confmode
  set namespace ::ns_${ved}
  set time [clock seconds]
  set oldtime [set ${namespace}::do_${i}(time)]
  set ${namespace}::do_${i}(time) $time
  set tdiff [expr double($time-$oldtime)]
  set status [lindex $args 1]
  switch $status {
    0 {set ${namespace}::do_${i}(status) "Running"}
    1 {set ${namespace}::do_${i}(status) "Never started"}
    2 {set ${namespace}::do_${i}(status) "Done"}
    3 {set ${namespace}::do_${i}(status) "Error"}
    4 {set ${namespace}::do_${i}(status) "Flushing"}
    default {set ${namespace}::do_${i}(status) [lindex $args 1]}
  }
  switch [lindex $args 2] {
    -1 {set ${namespace}::do_${i}(switch) "none"}
    0 {set ${namespace}::do_${i}(switch) "enabled"}
    1 {set ${namespace}::do_${i}(switch) "disabled"}
    default: {set ${namespace}::do_${i}(switch) [lindex $args 2]}
  }
  set susp [lindex $args 7]
  set clust [lindex $args 4]
  #set events [lindex $args 6]
  set kbytes [expr [lindex $args 5]*4]
  if {[lindex $args 3]>=7} {
    set buff [expr [lindex $args 8]]
    set buffs [expr [lindex $args 9]]
    set ${namespace}::do_${i}(buff) $buff
    set ${namespace}::do_${i}(rbuff) "von $buffs"
#     if {($buffs>0) && ($status!=1)} {
#       set ${namespace}::do_${i}(rbuff) [format {%.1f %%} [expr double($buff)/$buffs*100]]
#     } else {
#       set ${namespace}::do_${i}(rbuff) ---
#     }
  }  
  if {($oldtime>0) && ($tdiff>0)} {
    set ${namespace}::do_${i}(rsusp) \
        [format {%.1f /s} [expr ($susp-[set ${namespace}::do_${i}(susp)])/$tdiff]]
    set ${namespace}::do_${i}(rclust) \
        [format {%.1f /s} [expr ($clust-[set ${namespace}::do_${i}(clust)])/$tdiff]]
    set ${namespace}::do_${i}(rkbytes) \
        [format {%.1f /s} [expr ($kbytes-[set ${namespace}::do_${i}(kbytes)])/$tdiff]]
  }
  set ${namespace}::do_${i}(susp) [expr $susp]
  set ${namespace}::do_${i}(clust) [expr $clust]
  set ${namespace}::do_${i}(kbytes) [expr $kbytes]
}

proc init_di_perfvals {namespace i} {
  set ${namespace}::di_${i}(ok) 0
  set ${namespace}::di_${i}(status) unknown
  set ${namespace}::di_${i}(definition) ---
  set ${namespace}::di_${i}(susp) ---
  set ${namespace}::di_${i}(rsusp) ---
  set ${namespace}::di_${i}(clust) ---
  set ${namespace}::di_${i}(rclust) ---
  set ${namespace}::di_${i}(events) ---
  set ${namespace}::di_${i}(revents) ---
  set ${namespace}::di_${i}(kbytes) ---
  set ${namespace}::di_${i}(rkbytes) ---
}

proc init_do_perfvals {namespace i} {
  set ${namespace}::do_${i}(time) 0
  set ${namespace}::do_${i}(definition) ---
  set ${namespace}::do_${i}(status) unknown
  set ${namespace}::do_${i}(switch) unknown
  set ${namespace}::do_${i}(susp) ---
  set ${namespace}::do_${i}(rsusp) ---
  set ${namespace}::do_${i}(clust) ---
  set ${namespace}::do_${i}(rclust) ---
  set ${namespace}::do_${i}(events) ---
  set ${namespace}::do_${i}(revents) ---
  set ${namespace}::do_${i}(kbytes) ---
  set ${namespace}::do_${i}(rkbytes) ---
  set ${namespace}::do_${i}(buff) ---
  set ${namespace}::do_${i}(rbuff) ---
}

proc init_perfvals {namespace} {
  set ${namespace}::ro(time) 0
  # set ${namespace}::ro(trigger) ---
  set ${namespace}::ro(status) unknown
  set ${namespace}::ro(events) ---
  set ${namespace}::ro(revents) ---
  set ${namespace}::ro(susp) ---
  set ${namespace}::ro(rsusp) ---
  set ${namespace}::ro(evsize) ---
  set ${namespace}::ro(copies) ---
  set ${namespace}::ro(rcopies) ---
  foreach i [set ${namespace}::dataindom] {
    init_di_perfvals $namespace $i
  }
  foreach i [set ${namespace}::dataoutdom] {
    init_do_perfvals $namespace $i
  }
}
