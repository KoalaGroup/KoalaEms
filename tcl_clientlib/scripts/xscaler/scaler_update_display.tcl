# $ZEL: scaler_update_display.tcl,v 1.3 2011/11/26 02:01:55 wuestner Exp $
# copyright 1998 P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
# 
# global_num_channels  // total number of channels
# 
# scaler_cont($chan)
# scaler_cont_disp($chan)
# scaler_rate_disp($chan)
# scaler_max_disp($chan)
# scaler_time($chan)
# scaler_time_disp($chan)
# 
# ro_status(count)
# ro_status(rate_disp)
# ro_status(status_disp)
# ro_status(reldt_disp)
# ro_status(dt_disp)
# ro_status(status)
# 
# last_display_time
# global_setup(display_repeat)
# 
# global_comm_sequence      sequence number
# global_comm_reqnum(ist)   number of requests per sequence
# 

proc reset_vars {} {
  global global_num_channels last_display_time
  global scaler_time
  global scaler_time_last
  global scaler_time_disp
  global scaler_cont
  global scaler_cont_last
  global scaler_cont_disp
  global scaler_max_disp
  global scaler_rate_disp
  global ro_status
  global global_comm_sequence
  global global_comm_reqnum
  global deadtime

  set last_display_time 0
  for {set chan 0} {$chan<$global_num_channels} {incr chan} {
    set scaler_cont($chan) 0
    set scaler_cont_last($chan) 0
    set scaler_cont_disp($chan) ---
    set scaler_rate_disp($chan) ---
    set scaler_max_disp($chan) ---
    set scaler_time($chan) 0
    set scaler_time_last($chan) 0
    set scaler_time_disp($chan) 0
  }
  set ro_status(status) "unknown"
  set ro_status(status_disp) "unknown"
  set ro_status(time) 0
  set ro_status(time_disp) 0
  set ro_status(count) 0
  set ro_status(count_disp) ?
  set ro_status(reldt_disp) ?
  set ro_status(dt_disp) ?
  set deadtime(last_time) 0
  set global_comm_sequence 0
}

proc update_display {jetzt immer} {
  global global_num_channels last_display_time
  global global_setup
  global scaler_time
  global scaler_time_disp
  global scaler_cont
  global scaler_cont_disp
  global scaler_max_disp
  global scaler_rate_disp
  global ro_status
  global deadtime

  if {([expr $jetzt-$last_display_time]<$global_setup(display_repeat))&&!$immer} {
    return
  }

  if {$ro_status(status)=="running"} {
    if {("$ro_status(status_disp)"=="running") && ($ro_status(time_disp)!=0)
        && ($ro_status(time)>$ro_status(time_disp))} {
      set ro_status(rate_disp) [format {%7.1f} \
          [expr ($ro_status(count)-$ro_status(count_disp))\
          /($ro_status(time)-$ro_status(time_disp))]]
    } else {
      set ro_status(rate_disp) ?
    }
    set ro_status(status_disp) $ro_status(status)
    set ro_status(count_disp) $ro_status(count)
    set ro_status(time_disp) $ro_status(time)

    if {$global_setup(view_deadtime)} {
      set tm $scaler_time($global_setup(channel_measured))
      set tt $scaler_time($global_setup(channel_triggered))
      if {($deadtime(last_time)>0) && ($tm>$deadtime(last_time))} {
        if {$tm==$tt} {
          set cm [expr double($scaler_cont($global_setup(channel_measured)))]
          set ct [expr double($scaler_cont($global_setup(channel_triggered)))]
          set dm [expr $cm-$deadtime(last_measured)]
          set dt [expr $ct-$deadtime(last_triggered)]
          if {($dm>0) && ($dt>0)} {
            set td [expr $tm-$deadtime(last_time)]
            set ro_status(reldt_disp) [format {%.0f} [expr (1.-$dm/$dt)*100.]]
            set ro_status(dt_disp) [format {%.2f} [expr (1/$dm-1/$dt)*$td*1000]]
          } else {
            set ro_status(reldt_disp) ?
            set ro_status(dt_disp) ?
          }
          set deadtime(last_triggered) $ct
          set deadtime(last_measured) $cm
          set deadtime(last_time) $tm
        } else {
          output "timestamps of measured channel and triggered channel differ"
          output_append "daedtime display disabled"
          set global_setup(view_deadtime) 0
        }
      } else {
        if {$tm==$tt} {
          set deadtime(last_triggered) $scaler_cont($global_setup(channel_triggered))
          set deadtime(last_measured) $scaler_cont($global_setup(channel_measured))
          set deadtime(last_time) $tm
        } else {
          output "timestamps of measured channel and triggered channel differ"
        }
      }
    }
  } else {
    set ro_status(rate_disp) 0
    set ro_status(reldt_disp) ?
    set ro_status(dt_disp) ?
    set ro_status(status_disp) $ro_status(status)
    set ro_status(count_disp) $ro_status(count)
    set ro_status(time_disp) $ro_status(time)
  }

  for {set chan 0} {$chan<$global_num_channels} {incr chan} {
    if {($scaler_time_disp($chan)==0)
        || ($scaler_time($chan)<=$scaler_time_disp($chan))} {
      set scaler_max_disp($chan) 0
    } else {
      set rate [format {%7.1f} \
          [expr ($scaler_cont($chan)-$scaler_cont_disp($chan))\
          /($scaler_time($chan)-$scaler_time_disp($chan))]]
      set scaler_rate_disp($chan) $rate
      if {$rate>$scaler_max_disp($chan)} {
        set scaler_max_disp($chan) $rate
      }
    }
    set scaler_cont_disp($chan) [format {0x%08x} $scaler_cont($chan)]
    set scaler_time_disp($chan) $scaler_time($chan)
  }
  if {!$immer} {
    set last_display_time $jetzt
  }
  update idletasks
  scaler_win_scale_canvas
}
