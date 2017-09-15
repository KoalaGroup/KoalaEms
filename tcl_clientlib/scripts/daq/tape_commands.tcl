# $ZEL: tape_commands.tcl,v 1.8 2003/02/04 19:28:00 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc ask_for_tape {command} {
  set win .tape_question
  set text $command
  set res [tk_dialog $win "TAPE Question" $text \
        questhead 1 {proceed} {abort}]
  update idletasks
  return $res
}

proc ask_for_tape_no {} {
  set win .tape_question
  set text {DAQ is running (or not initialized), tape commands are disabled.}
  tk_dialog $win "TAPE Message" $text warning 0 {OK}
}

proc find_tape_cancel {w} {
  global global_find_tape_var
  destroy $w
  set global_find_tape_var -1
}

proc find_tape_ok {w} {
  global global_find_tape_var
  destroy $w
  set global_find_tape_var 0
}

proc find_tape_destroyed {w} {
  global global_find_tape_var
  #output "$w destroyed" tag_red
  destroy $w
  set global_find_tape_var -2
}

proc find_tape {} {
  global global_dataoutlist global_find_tape_var
  global tape_ved tape_idx tape_type
  global global_daq

  set num 0
  set list {}
  foreach ved [array names global_dataoutlist] {
    foreach idx $global_dataoutlist($ved) {
      set key ${ved}_${idx}
      global global_dataout_$key
      if [set global_dataout_${key}(istape)] {
        if {$global_daq(status)=="running"} {
          if [catch {set s [$ved dataout status $idx]} mist] {
            set usable 0
          } else {
            set usable [lindex $s 2]
          }
        } else {
          set usable 1
        }
      if {$usable} {
        set tape_ved($num) $ved
        set tape_idx($num) $idx
        set tape_type($num) [string trim [set global_dataout_${key}(tape)]]
        lappend list "[$tape_ved($num) name] $tape_idx($num) $tape_type($num)"
        incr num
        }
      }
    }
  }
  if {$num<1} {
    output "there are no tapes available, no tape commands possible" tag_red
    return {}
  }
  if {$num==1} {
    return 0
  }
  # more than one tape, must ask user
  set res [selector .ask_for_tape "Which tape?" "ask for tape" $list 0 0 extended]
  #output "selector: res=$res"
  if {$res<0} {set res {}}
  return $res
}

proc tape_wind_eod {} {
  global global_daq
  global tape_ved tape_idx tape_type

  if {$global_daq(status)!="idle"} {
    ask_for_tape_no
    return
  }
  set tapes [find_tape]

  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    set res [ask_for_tape "Wind $type (ved [$ved name] do $do) to end of data?"]
    if {$res} break
    #if [catch {$ved dataout wind $do end} mist]
    if [catch {$ved command1 TapeSpace $do 3 0} mist] {
      output "wind tape $type (ved [$ved name] do $do) to end of data: $mist" tag_red
    } else {
      output "wind tape $type (ved [$ved name] do $do) to end of data"
    }
  }
}

proc tape_mark {} {
  global global_daq
  global tape_ved tape_idx tape_type

  if {$global_daq(status)!="idle"} {
    ask_for_tape_no
    return
  }
  set tapes [find_tape]

  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    set res [ask_for_tape "Write Filemark to $type (ved [$ved name] do $do)?"]
    if {$res} return

    #if [catch {$ved dataout wind $do 0} mist]
    if [catch {$ved command1 TapeFilemark $do 1 1} mist] {
      output "write filemark to $type (ved [$ved name] do $do): $mist" tag_red
    } else {
      output "write filemark to $type (ved [$ved name] do $do)"
    }
  }
}

proc tape_rewind {} {
  global global_daq
  global tape_ved tape_idx tape_type

  set tapes [find_tape]

  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    set res [ask_for_tape "Rewind Tape $type (ved [$ved name] do $do)?"]
    if {$res} return

    #if [catch {$ved dataout wind $do begin} mist]
    if [catch {$ved command1 TapeRewind $do 1} mist] {
      output "rewind tape $type (ved [$ved name] do $do): $mist" tag_red
    } else {
      output "rewind tape $type (ved [$ved name] do $do)"
    }
  }
}

proc tape_unload {} {
  global global_daq
  global tape_ved tape_idx tape_type

  set tapes [find_tape]
  
  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    set res [ask_for_tape "Rewind and Unload Tape $type (ved [$ved name] do $do)?"]
    if {$res} return

    if [catch {$ved command "TapeAllowRemoval $do"} mist] {
      output "Unload tape $type (ved [$ved name] do $do): $mist" tag_red
    } else {
      #if [catch {$ved dataout wind $do -10000} mist]
      if [catch {$ved command1 TapeUnload $do 1} mist] {
        output "Unload tape $type (ved [$ved name] do $do): $mist" tag_red
      } else {
        output "unload tape $type (ved [$ved name] do $do)"
      }
    }
  }
}

proc tape_load {} {
  global global_daq
  global tape_ved tape_idx tape_type

  if {$global_daq(status)!="idle"} {
    ask_for_tape_no
    return
  }
  set tapes [find_tape]

  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    if [catch {$ved command1 TapeLoad $do 1} mist] {
      output "Load tape $type (ved [$ved name] do $do): $mist" tag_red
    } else {
      output "Load tape $type (ved [$ved name] do $do)"
    }
  }
}

proc tape_erase {} {
  global global_daq
  global tape_ved tape_idx tape_type

  if {$global_daq(status)!="idle"} {
    ask_for_tape_no
    return
  }
  set tapes [find_tape]

  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    set res [ask_for_tape "Erase Tape $type (ved [$ved name] do $do)?"]
    if {$res} return

    set res [ask_for_tape "Are You REALLY shure to erase tape $type (ved [$ved name] do $do) ???"]
    if {$res} return

    if [catch {$ved command1 TapeErase $do 1} mist] {
      output "erase tape $type (ved [$ved name] do $do): $mist" tag_red
    } else {
      output "erase tape $type (ved [$ved name] do $do)"
    }
  }
}

proc tape_disable {} {
  global global_daq
  global tape_ved tape_idx tape_type

  set tapes [find_tape]

  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    set res [ask_for_tape "Disable Tape $type (ved [$ved name] do $do)?"]
    if {$res} return

    if [catch {$ved dataout enable $do 0} mist] {
      output "disable tape $type (ved [$ved name] do $do): $mist" tag_red
    } else {
      output "tape $type (ved [$ved name] do $do disabled)" tag_orange
      set_tapedisplay_disabled $ved $do 1
    }
  }
}

proc tape_enable {} {
  global global_daq
  global tape_ved tape_idx tape_type

  set tapes [find_tape]

  foreach i $tapes {
    set ved $tape_ved($i)
    set do $tape_idx($i)
    set type $tape_type($i)

    set res [ask_for_tape "Enable Tape $type (ved [$ved name] do $do)?"]
    if {$res} return

    if [catch {$ved dataout enable $do 1} mist] {
      output "enable tape $type (ved [$ved name] do $do): $mist" tag_red
    } else {
      output "tape $type (ved [$ved name] do $do enabled)" tag_orange
      set_tapedisplay_disabled $ved $do 0
    }
  }
}

