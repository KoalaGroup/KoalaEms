#
# global vars in this file:
#
# window global_output(text)
# boolean global_output(append)
# boolean global_output(wrap)
#

proc output_wrap {win color} {
  global global_output
  if $global_output(wrap) {
    set global_output(wrap) 0
    $global_output(text) config -wrap none
    $win config -background $color
  } else {
    set global_output(wrap) 1
    $global_output(text) config -wrap word
    $win config -background black
  }
}

proc create_log {w} {
  global global_output global_setup global_flog
  frame $w -relief flat -borderwidth 0
  set global_output(text) [text $w.t -relief ridge -borderwidth 2\
      -wrap none\
      -state disabled\
      -yscrollcommand "$w.sv set"\
      -xscrollcommand "$w.sh set"\
      -font -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*]
  $global_output(text) tag configure anfang -spacing1 2m
  $global_output(text) tag configure any -lmargin2 5m
  scrollbar $w.sv -width 10 -command "$w.t yview"
  scrollbar $w.sh -width 10 -orient horizontal -command "$w.t xview"
  frame $w.f -relief flat -borderwidth 0
  bind $w.f <1> "output_wrap $w.f [$w.f cget -background]"
  grid $w.t -column 0 -row 0 -sticky nsew
  grid $w.sv -column 1 -row 0 -sticky ns
  grid $w.sh -column 0 -row 1 -sticky ew
  grid $w.f -column 1 -row 1 -sticky nsew
  grid rowconfigure $w 0 -weight 1
  grid columnconfigure $w 0 -weight 1
  set global_output(append) 0
  set global_output(wrap) 0
  set global_flog ""
  if {$global_setup(auto_log)} {
    if [catch {set global_flog [open $global_setup(logfilename) \
              {RDWR CREAT} 0640]} mist] {
      output $mist
      set global_flog ""
    } else {
      timestamp 1
      output_append "logfile \"$global_setup(logfilename)\" open"
    }
  }
}

proc close_log {} {
  global global_flog
  if {$global_flog!=""} {
    puts $global_flog "  -- [clock format [clock seconds]] --"
    puts $global_flog "  log closed"
    close $global_flog
    }
  set global_flog ""
}

proc truncate_log {} {
  global global_output global_setup
  set idx [$global_output(text) index end]
  set lines [lindex [split $idx .] 0]
  if {$lines>$global_setup(loglength)} {
    $global_output(text) delete 1.0 [expr $lines-$global_setup(loglength)].0 
  }
}

proc output {text} {
  global global_output global_flog

  $global_output(text) config -state normal
  truncate_log
  if "$global_output(append)" {
    set tag anfang
  } else {
    set tag {}
    set global_output(append) 1
  }
  set ttext [string trim $text]
  if {$global_flog!=""} {
    puts $global_flog ""
    puts $global_flog $ttext
    flush $global_flog
  }
  $global_output(text) insert end [string index $ttext 0] "$tag any"
  $global_output(text) insert end [string range $ttext 1 end] any
  $global_output(text) insert end "\n"
  $global_output(text) see end
  $global_output(text) config -state disabled
}

proc output_append {text} {
  global global_output global_flog

  if {$global_flog!=""} {
    puts $global_flog $text
    flush $global_flog
  }
  $global_output(text) config -state normal
  $global_output(text) insert end $text any
  $global_output(text) insert end "\n"
  $global_output(text) see end
  $global_output(text) config -state disabled
}

proc output_append_nonl {text} {
  global global_output global_flog

  if {$global_flog!=""} {
    puts -nonewline $global_flog $text
    flush $global_flog
  }
  $global_output(text) config -state normal
  $global_output(text) insert end $text any
  $global_output(text) see end
  $global_output(text) config -state disabled
}

proc output_list {text} {
  global global_output global_flog

  $global_output(text) config -state normal
  if "$global_output(first)==0" {
    $global_output(text) insert end "\n"
    set global_output(first) 0
  }
  if {$global_flog!=""} {
    foreach i $text {
      puts $global_flog $i
    }
    flush $global_flog
  }
  foreach i $text {
    $global_output(text) insert end $i
    $global_output(text) insert end "\n"
    $global_output(text) see end
  }
  $global_output(text) config -state disabled
}
