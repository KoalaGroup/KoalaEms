# $Id: perf_request.tcl,v 1.1 2000/07/15 21:37:02 wuestner Exp $
# copyright:
# 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

proc got_nostatus {space conf} {
namespace eval $space {
  LOG::put "got no status from VED $name"
}
}

proc got_status {space seq size args} {
LOG::put "got_status: sequence $seq" tag_orange
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
  
  if {$damals>0} {
    set m_diff [expr double($eventcount-$old_eventcount)]
    set r_diff [expr double($rejected-$old_rejected)]
    set t_diff [expr double($m_diff+$r_diff)]
    set difftime [expr double($jetzt-$damals)]
    if {$difftime==0} {
      LOG::put "difftime=0"
    } else {
      set mrate [format {%.1f} [expr $m_diff/$difftime]]
      set trate [format {%.1f} [expr $t_diff/$difftime]]
      if {($t_diff>0) && ($r_diff>0)} {
        set deadtime_perc [format {%.1f} [expr $r_diff/$t_diff*100.]]
        set deadtime_ms [format {%.3f} [expr (1./$m_diff-1./$t_diff)*$difftime*1000]]
      } else {
        set deadtime_perc ---
        set deadtime_ms ---
      }
    }
  }
  set old_eventcount $eventcount
  set old_rejected $rejected
  set damals $jetzt

  set numstruct [lindex $data 3]
  set data [lrange $data 4 end]
  for {} {$numstruct>0} {incr numstruct -1} {
    switch [lindex $data 0] {
      1 {set canvas {}; set type ldt}
      2 {set canvas $dtc; set type tdt}
      4 {set canvas {}; set type gaps}
      default {set canvas {}; LOG::put "received array code [lindex $data 0]" tag_red}
    }
    LOG::put "data: $data"
    set num_head 13
    set num_arr [lindex $data 11]
    set num_null [lindex $data 12]
    set num [expr $num_head+$num_arr-$num_null]
#     LOG::append "num_head: $num_head"
#     LOG::append "num_arr: $num_arr"
#     LOG::append "num_null: $num_null"
#     LOG::append "num: $num"
    if {$canvas!={}} {
      LOG::put "type: $type; sequence $seq" tag_orange
      LOG::append "entries: [lindex $data 1]"
      LOG::append "ovl: [lindex $data 2]"
      LOG::append "min: [lindex $data 3]"
      LOG::append "max: [lindex $data 4]"
      LOG::append "sum: [lindex $data 5]"
      LOG::append "sum: [lindex $data 6]"
      LOG::append "qsum: [lindex $data 7]"
      LOG::append "qsum: [lindex $data 8]"
      LOG::append "histsize: [lindex $data 9]"
      LOG::append "histscale: [lindex $data 10]"
      LOG::append "numvalues: [lindex $data 11]"
      LOG::append "leading zeros: [lindex $data 12]"
      set scaling [expr [lindex $data 10]]; # *arg[4] of GetSyncStatist
      set binwidth [expr $scaling]; # * width of original bin (100 ns?)
      set fout [open "plot.data" w]
      set leading_zeros [lindex $data 12]
      set num_values [lindex $data 11]
      for {set i 0} {$i<$leading_zeros} {incr i} {
        puts $fout "$i 0"
      }
      set subdata [lrange $data 13 end]
      for {set i $leading_zeros; set idx 0} {$i<$num_values} {incr i; incr idx} {
        puts $fout "$i [expr [lindex $subdata $idx]/10000.]"
      }
      close $fout
      set plotstr {}
      append plotstr "set term tkcanvas\n"
      append plotstr "set nokey\n"
      #append plotstr "set title \"nix\"\n"
      append plotstr "set xlabel \"us\"\n"
      #append plotstr "set mxtics 10\n"
      #append plotstr "set mytics 5\n"
      append plotstr "set grid\n"
      append plotstr "set output 'plot.file'\n"
      append plotstr "plot 'plot.data' w l\n"
      #append plotstr "set xrange \[1.:2.\]\n"
      if [catch {exec gnuplot << $plotstr} mist] {
        LOG::put $mist
      }
      source plot.file
      gnuplot $canvas
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
  $ved command {GetSyncStatist {7 0 0 0}} \
    : got_status [namespace current] $seq ? got_nostatus [namespace current]
}
}
