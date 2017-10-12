# $ZEL: perf_request.tcl,v 1.1 2000/07/15 21:37:02 wuestner Exp $
# copyright:
# 1999 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc got_nostatus {space conf} {
namespace eval $space {
  LOG::put "got no status from VED $name"
}
}

proc got_status {space seq size args} {
#LOG::put "got_status: sequence $seq" tag_orange
set ${space}::size $size
set ${space}::data $args
set ${space}::seq $seq
namespace eval $space {
  set jetzt [clock seconds]
  #LOG::put "got status from VED $name: $data"
  #LOG::put "size=$size"
  #LOG::put "Eventcount=[lindex $data 0]"
  #LOG::put "rejected Triggers=[lindex $data 2]"
  #LOG::put "rest=[lindex $data 3]"
  set eventcount [lindex $data 0]
  set rejected [expr ([lindex $data 1]<<32)+[lindex $data 2]]
  #LOG::put "rejected =$rejected"
  
  if {$is_master} {
    if {$damals>0} {
      set m_diff [expr double($eventcount-$old_eventcount)]
      set r_diff [expr double($rejected-$old_rejected)]
      set t_diff [expr double($m_diff+$r_diff)]
      set difftime [expr double($jetzt-$damals)]
      if {$difftime==0} {
        LOG::put "difftime=0"
      } else {
        global global_mrate global_trate
        global global_deadtime_perc global_deadtime_ms
        # measured rate
        set global_mrate [format {%.1f} [expr $m_diff/$difftime]]
        # triggered rate
        set global_trate [format {%.1f} [expr $t_diff/$difftime]]
        if {($t_diff>0) && ($r_diff>0)} {
          # relative deadtime
          set global_deadtime_perc \
              [format {%.1f} [expr $r_diff/$t_diff*100.]]
          # absolute deadtime
          set global_deadtimec_ms \
              [format {%.3f} [expr (1./$m_diff-1./$t_diff)*$difftime*1000]]
        } else {
          set global_deadtime_perc ---
          set global_deadtimec_ms ---
        }
      }
    }
    set old_eventcount $eventcount
    set old_rejected $rejected
    set damals $jetzt
  }

  set numstruct [lindex $data 3]
  set data [lrange $data 4 end]
  for {} {$numstruct>0} {incr numstruct -1} {
    # 1: local dead time; 2: total deadtime; 4: trigger distribution
    switch [lindex $data 0] {
      1 {set type ldt}
      2 {set type tdt}
      4 {set type gaps}
      default {LOG::put "received array code [lindex $data 0]" tag_red}
    }
    #LOG::put "data: $data"
    if {$type!="gaps"} {
      set num_head 12
      set num_arr [lindex $data 11]
      if {$num_arr} {
        set num_null [lindex $data 12]
        set num [expr $num_head+1+$num_arr-$num_null]
      } else {
        set num $num_head
      }

      set entries   [lindex $data 1]
      set overflows [lindex $data 2]
      set minimum   [lindex $data 3]
      set maximum   [lindex $data 4]
      set s1 [lindex $data 5]
      set s2 [lindex $data 6]
      set sum       [expr $s1*0x100000000+$s2]
      set qsum      [expr [lindex $data 7]*0x100000000+[lindex $data 7]]
      if {$is_master} {
        LOG::put "ved [$ved name]; seq $seq"
        LOG::append "entries: $entries"
        LOG::append "s1=$s1 s2=$s2)"
        #LOG::append "sum:     $sum ([format {%x %x} $s1 $s2])"
      }
  #     LOG::append "entries: $entries"
  #     LOG::append "ovl:     $overflows"
  #     LOG::append "min:     $minimum"
  #     LOG::append "max:     $maximum"
  #     LOG::append "sum:     $sum"
  #     LOG::append "qsum:    $qsum"
      # dead time is measured in terms of 100 ns
      # dt=sum/(100ns/s)/num
      set dt [expr double($sum)/double($entries)/10000.]
      if {$type=="tdt"} {
        set global_deadtimem_ms [format {%.3f ms} $dt]
      } else {
        set ldt [format {%.3f ms} $dt]
      }
    }
    set data [lrange $data $num end]
  }
}
}

proc send_request {space} {
namespace eval $space {
  if {![info exists [namespace current]::seq]} {
    variable seq 0
  }
  incr seq
  if {$is_master} {
    $ved command {GetSyncStatist {3 0 -1 0}} \
      : got_status [namespace current] $seq ? got_nostatus [namespace current]
  } else {
    $ved command {GetSyncStatist {1 0 -1 0}} \
      : got_status [namespace current] $seq ? got_nostatus [namespace current]
  }
}
}

proc ignore {args} {
  LOG::put "ignored: $args"
}

proc reset_statistics {space} {
namespace eval $space {
  if {$is_master} {
    $ved command {ClearSyncStatist {3}} \
      : ignore ? ignore
  } else {
    $ved command {ClearSyncStatist {1}} \
      : ignore ? ignore
  }
}
}
