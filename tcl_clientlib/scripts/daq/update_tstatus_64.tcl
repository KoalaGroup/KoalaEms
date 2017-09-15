# $ZEL: update_tstatus_64.tcl,v 1.8 2007/04/15 13:15:09 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc update_tstatus_64 {key status} {
  global t0 clockformat
  global global_tapestat
  global global_tapestat_ved
  global global_tapestat_do
  global global_tapestat_var_time
  global global_tapestat_var_rest
  global global_tapestat_var_end
  global global_tapestat_var_rate
  global global_tapestat_var_host
  global global_tapestat_var_host_letzt
  global global_tapestat_var_remaining
  global global_tapestat_var_remaining_letzt
  global global_tapestat_var_tape
  global global_tapestat_var_tape_letzt
  global global_tapestat_var_comp
  global global_tapestat_var_rewrites
  global global_tapestat_var_err_corr
  global global_tapestat_var_pos
  global global_tapestat_var_temp
  global global_tapestat_frame_pos
  global global_tapestat_frame_color

  global global_dataout_$key

  #output "tape $key ([set global_dataout_${key}(tape)]):"
  #output_append "$status"
  switch [set global_dataout_${key}(type)] {
    DLT4000 -
    DLT7000 -
    DLT8000 {
      set len [llength $status]
      #output "DLT\[47\]000: len=$len"
      if {$len>=20} {
        set jetzt [doubletime]

        set mbhost [lindex $status 13]
        set bhost [lindex $status 14]
        set var [expr $mbhost*1048576+$bhost]

        set mbtape [lindex $status 15]
        set btape [lindex $status 16]
        set tvar [expr $mbtape*1048576+$btape]
        set ftvar [expr $mbtape*1048576.+$btape]

        set global_tapestat_var_tape($key) [xformat $tvar]

        if {$global_tapestat_var_time($key)>0} {
          set global_tapestat_var_rate($key) [
              xformat [
                  expr {int(($var-$global_tapestat_var_host_letzt($key))
                    /($jetzt-$global_tapestat_var_time($key)))}]]
          set trate [
                  expr {int(($tvar-$global_tapestat_var_tape_letzt($key))
                    /($jetzt-$global_tapestat_var_time($key)))}]
        } else {
          set global_tapestat_var_rate($key) ???
          set trate 0
        }
        set global_tapestat_var_host_letzt($key) $var
        set global_tapestat_var_tape_letzt($key) $tvar
        set global_tapestat_var_host($key) [xformat $var]
        set global_tapestat_var_time($key) $jetzt
        set global_tapestat_var_comp($key) [
            format {%.2f} [expr [lindex $status 12]/100.]]
        set pos {}
        set BOP [lindex $status 17]
        set EOP [lindex $status 18]
        if {!$BOP && !$EOP} {
          $global_tapestat_frame_pos($key) configure -background\
              $global_tapestat_frame_color
        } else {
          if {$BOP} {
            append pos {(BOP) }
            $global_tapestat_frame_pos($key) configure -background yellow
          }
          if {$EOP} {
            append pos {(EOP) }
            $global_tapestat_frame_pos($key) configure -background red
          }
        }
        append pos [xformat [lindex $status 19]]
        set global_tapestat_var_pos($key) $pos
        set global_tapestat_var_rewrites($key) [xformat [lindex $status 7]]
        set global_tapestat_var_err_corr($key) [xformat [lindex $status 8]]
        if {$len>=21} {
          # DLT7000 remaining tape
          set rest [expr [lindex $status 20]*4096]
          set global_tapestat_var_rest($key) [xformat $rest]
          set end_secs 0
          if {$trate>0} {
            set end_secs [calculate_tapeend $key $ftvar $rest [expr [doubletime]-$t0]]
            #set rest_secs [expr $rest/$trate]
            #set end_secs [expr [clock seconds] + $rest_secs]
          }
          if {$end_secs} {
            set global_tapestat_var_end($key) [clock format $end_secs -format $clockformat]
          } else {
            set global_tapestat_var_end($key) unknown
          }
        }
        if {$len>=22} {
          # DLT8000 drive temperature
          set global_tapestat_var_temp($key) [format {%d} [lindex $status 21]]
        }
      } else {
        set global_tapestat_var_rest($key) ???
        set global_tapestat_var_host($key) ???
        set global_tapestat_var_rate($key) ???
        set global_tapestat_var_host_letzt($key) 0
        set global_tapestat_var_time($key) 0
        set global_tapestat_var_tape($key) ???
        set global_tapestat_var_comp($key) ???
        set global_tapestat_var_pos($key) ???
        $global_tapestat_frame_pos($key) configure -background\
              $global_tapestat_frame_color
        set global_tapestat_var_rewrites($key) ???
        set global_tapestat_var_err_corr($key) ???
      }
      if {($len>=15) && ([lindex $status 1]!=0)} {
        output "Tapestatus:" tag_red
        output_append "  error code           : [lindex $status 0]"
        output_append "  sense key            : [lindex $status 1]"
        output_append "  add. sense code      : [lindex $status 2]"
        output_append "  add. sense code qual.: [lindex $status 3]"
      }
    }
    EXABYTE {
      set len [llength $status]
      #output "EXABYTE: len=$len"
      #output "$status"
      if {$len>=4} {
        set jetzt [doubletime]
        set remlen [lindex $status 0]
        set errcount [lindex $status 1]
        set bytes [lindex $status 2]
        set addsensecode [lindex $status 3]
        set byte2 [expr ($bytes>>24)&0xff]
        set byte19 [expr ($bytes>>16)&0xff]
        set byte20 [expr ($bytes>>8)&0xff]
        set byte21 [expr $bytes&0xff]
        # Tape Not Present
        set TNP [expr $byte19&0x2]
        # Logical Begin Of Tape
        set LBOT [expr $byte19&0x1]
        # Write Protected
        set WP [expr $byte20&0x20]
        # needs to be Cleaned
        set CLN [expr $byte21&0x8]
        # Physical End Of Tape
        set PEOT [expr $byte21&0x4]
        set global_tapestat_var_remaining($key) [xformat $remlen]
        if {$global_tapestat_var_time($key)>0} {
          set rate [
            expr {
              int(($global_tapestat_var_remaining_letzt($key)-$remlen)
              /($jetzt-$global_tapestat_var_time($key)))
            }
          ]
          set global_tapestat_var_rate($key) [xformat $rate]
        } else {
          set global_tapestat_var_rate($key) ???
          set rate 0
        }
        if {$rate>0} {
          set rest_secs [expr $remlen/$rate]
          set end_secs [expr [clock seconds] + $rest_secs]
          set global_tapestat_var_end($key) [clock format $end_secs -format $clockformat]
        } else {
          set global_tapestat_var_end($key) unknown
        }
        set global_tapestat_var_remaining_letzt($key) $remlen
        set global_tapestat_var_time($key) $jetzt
        set global_tapestat_var_rewrites($key) [xformat $errcount]
        set pos {}
        if {$TNP}  {append pos "(no tape) "}
        if {$LBOT} {append pos "(LBOT) "}
        if {$WP}   {append pos "(protected) "}
        if {$CLN}  {append pos "(cleaning required) "}
        if {$PEOT} {append pos "(PEOT) "}
        if {!$LBOT && !$PEOT && !$CLN} {
          $global_tapestat_frame_pos($key) configure -background\
              $global_tapestat_frame_color
        } else {
          if {$LBOT || $CLN} {
            $global_tapestat_frame_pos($key) configure -background yellow
          }
          if {$PEOT} {
            $global_tapestat_frame_pos($key) configure -background red
          }
        }
        # puts "status: $status"
        if {[llength $status]>6} {append pos [xformat [lindex $status 6]]}
        set global_tapestat_var_pos($key) $pos
      } else {
        output "Tapestatus EXABYTE: len=$len (not 4)" tag_red
        set global_tapestat_var_remaining($key) ???
        set global_tapestat_var_rate($key) ???
        set global_tapestat_var_rewrites($key) ???
      }
    }
    default {
      set len [llength $status]
      #output "UNKNOWN: len=$len"
      if {$len>=8} {
        set pos {}
        set EOM [lindex $status 3]
        set BOP [expr $EOM & 1]
        set EOP [expr $EOM & 2]
        if {!$BOP && !$EOP} {
          $global_tapestat_frame_pos($key) configure -background\
              $global_tapestat_frame_color
        } else {
          if {$BOP} {
            append pos {(BOP) }
            $global_tapestat_frame_pos($key) configure -background yellow
          }
          if {$EOP} {
            append pos {(EOP) }
            $global_tapestat_frame_pos($key) configure -background red
          }
        }
        
        append pos [xformat [lindex $status 4]]
        set global_tapestat_var_pos($key) $pos
      } else {
        set global_tapestat_var_pos($key) ???
      }
    }
  }
}
