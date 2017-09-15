proc getname {id} {
  global commu names
  if [info exists names($id)] {
    return $names($id)
  } else {
  set confmode [ems_confmode synchron]
    if [catch {set n [$commu command1 globalvedname $id | string]}] {
      set n "<unknown ($id)>"
    } else {
      set names($id) $n
    }
    return $n
  ems_confmode $confmode
  }
}

proc getved {name} {
  set veds [ems_openveds]
  foreach v $veds {
    if {[$v name]==$name} {
      return $v
    }
  }
 return "" 
}

proc timestamp {immer} {
  global global_lasttime
  set seconds [clock seconds]
  if {$immer || ([expr $global_lasttime+60]<$seconds)} {
    output "  -- [clock format $seconds] --"
    set global_lasttime $seconds
  }
}

proc write_delimiter {} {
  timestamp 1
  output_append "==============================================================================="
}

proc print_unsol {typ head data} {
  timestamp 0
  if {([lindex $head 4]&0x20)==0} {
    output "message from commu"
    output_append "typ=$typ head=$head data=$data"
  } else {
    set vedid [lindex $head 2]
    set vedname [getname $vedid]
    output "unsol $typ:"
    output_append "  data= $data"
    output_append "  ved = $vedname"
  }
}

proc RuntimeError {head data} {
  timestamp 0
  set vedname [getname [lindex $head 2]]
  output "VED $vedname: RuntimeError"
  set rtcode [lindex $data 0]
  set data [lreplace $data 0 0]
  switch $rtcode {
    0 {output_append "  no error"}
    1 {
      output_append "  Fehler im Ausgabegeraet; dataout [lindex $data 0]"
      set data [lreplace $data 0 0]
    }
    2 {output_append "  Fehler bei der Triggersynchronisation"}
    3 {output_append "  Falsche Eventnummer"}
    4 {output_append "  Fehler beim Ausfuehren einer Prozedurliste"}
    5 {output_append "  anderer Fehler"}
    6 {
      output_append "  Fehler im Eingabegeraet; datain [lindex $data 0]"
      set data [lreplace $data 0 0]
    }
    default {output_append "  unknown rt_code $rtcode"}
  }
  if ([llength $data]>0) {
    output_append "  additional data:\{$data\}"
  }
}

proc LAM {head data} {
  set vedname [getname [lindex $head 2]]
  output "LAM from VED $vedname:"
  output_append "  head= \{$head\} data=\{$data\}"
}

proc print_StatusChanged {head data} {
  global Object Domain status_action

  timestamp 0
  set vedname [getname [lindex $head 2]]
  set ved [getved $vedname]
  set action [lindex $data 0]
  set object [lindex $data 1]
  output "StatusChanged from VED $vedname:"
  output_append_nonl "  $status_action($action)"
  set data [lreplace $data 0 1]
  switch $object {
    2 {
      set domain [lindex $data 0]
      set idx [lindex $data 1]
      output_append " domain $Domain($domain) $idx"
      set data [lreplace $data 0 1]
      if {$ved!=""} {
        switch $domain {
          1 {        
            if {![catch {set stat [$ved modullist upload]}]} {
              output_append "  \{$stat\}"
            } 
          }
          3 {
            if {![catch {set stat [$ved trigger upload]} mist]} {
              output_append "  \{$stat\}"
            } else {
              output_append "  upload trigger: $mist"
            }
          }
          5 {
            if {![catch {set stat [$ved datain upload $idx]}]} {
              output_append "  \{$stat\}"
            } 
          }
          6 {
            if {![catch {set stat [$ved dataout upload $idx]}]} {
              output_append "  \{$stat\}"
            } 
          }
        }
      }
    }
    6 {
      set idx [lindex $data 0]
      if {$idx==-1} {
        output_append " all dataouts"
      } else {
        output_append " dataout $idx"
      }
      set data [lreplace $data 0 0]
    }
    5 {
      set pi [lindex $data 0]
      set idx [lindex $data 1]
      if {$pi==1} {
        output_append " readout"
        if {$ved!=""} {
          if {![catch {set stat [$ved readout status]}]} {
            output_append "  \{$stat\}"
          } 
        }
      } else {
        output_append " LAM $idx"
      }
      set data [lreplace $data 0 1]
    }
    3 {
      set idx [lindex $data 0]
      output_append " instrumentation system $idx"
      set data [lreplace $data 0 0]
    }
    0 -
    1 -
    4 -
    default {output_append " $Object($object)"}
    }
  if {[llength $data]} {output_append "  additional data:\{$data\}"
  }
}

proc ServerDisconnect {v} {
  timestamp 1
  output_append "  ###  [$v name] disconnected   ###"
  $v close
}

proc Bye {h d} {
  timestamp 1
  output_append "  ###  commu says Bye   ###"
  ems_disconnect
}

proc setup_unsolcommands {} {
  global status_action Object Domain

  set status_action(0) none
  set status_action(1) created
  set status_action(2) deleted
  set status_action(3) changed
  set status_action(4) started
  set status_action(5) stopped
  set status_action(6) reset
  set status_action(7) resumed
  set status_action(8) enabled
  set status_action(9) disabled
  set status_action(10) finished

  set Object(0) null
  set Object(1) ved
  set Object(2) domain
  set Object(3) {instrumentation system}
  set Object(4) variable
  set Object(5) {program invocation}
  set Object(6) dataout

  set Domain(0) null
  set Domain(1) Modullist
  set Domain(2) LAMproclist
  set Domain(3) Trigger
  set Domain(4) Event
  set Domain(5) Datain
  set Domain(6) Dataout

  ems_unsolcommand ServerDisconnect {ServerDisconnect %v}
  ems_unsolcommand Bye {Bye %h %d}

  ems_unsolcommand Data {print_unsol Data %h %d}
  ems_unsolcommand LAM {LAM %h %d}
  ems_unsolcommand LogMessage {print_unsol LogMessage %h %d}
  ems_unsolcommand LogText {print_unsol LogText %h %d}
  ems_unsolcommand Nothing {print_unsol Nothing %h %d}
  ems_unsolcommand Patience {print_unsol Patience %h %d}
  ems_unsolcommand RuntimeError {RuntimeError %h %d}
  ems_unsolcommand StatusChanged {print_StatusChanged %h %d}
  ems_unsolcommand Test {print_unsol Test %h %d}
  ems_unsolcommand Text {print_unsol Text %h %d}
  ems_unsolcommand Warning {print_unsol Warning %h %d}
}

proc fehler {x} {
  break
}
