# $Id: log.tcl,v 1.1 2000/07/15 21:34:38 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_setup(loglength)
#
# window global_output(text)
# boolean global_output(wrap)
# window global_output(sv)
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

proc create_images {} {
  global EMSTCL_HOME
  image create bitmap warnung -foreground red -data {
#define info_full_width 8
#define info_full_height 21
static unsigned char info_full_bits[] = {
   0x3c, 0x3e, 0x3e, 0x3e, 0x1c, 0x00, 0x00, 0x3f, 0x3f, 0x3e, 0x3c, 0x3c,
   0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0xff, 0xff, 0xff};
}
}

proc create_log {w} {
  global global_output global_setup
  global global_verbose

  create_images
  frame $w -relief flat -borderwidth 0
  set global_output(text) [text $w.t -relief ridge -borderwidth 3\
      -wrap none\
      -state disabled\
      -yscrollcommand "$w.sv set"\
      -xscrollcommand "$w.sh set"\
      -font -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*]
  $global_output(text) tag configure anfang -spacing1 2m
  $global_output(text) tag configure any -lmargin2 5m
  $global_output(text) tag configure tag_red -foreground red
  $global_output(text) tag configure tag_green -foreground #007000
  $global_output(text) tag configure tag_blue -foreground blue
  $global_output(text) tag configure tag_orange -foreground #fe6800
  $global_output(text) tag configure tag_yellow -foreground #ece800
  $global_output(text) tag configure tag_under -underline 1
  $global_output(text) tag configure tag_strike -overstrike 1
  scrollbar $w.sv -width 10 -command "$w.t yview"
  set global_output(sv) $w.sv
  scrollbar $w.sh -width 10 -orient horizontal -command "$w.t xview"
  frame $w.f -relief flat -borderwidth 0
  bind $w.f <1> "output_wrap $w.f [$w.f cget -background]"
  grid $w.t -column 0 -row 0 -sticky nsew
  grid $w.sv -column 1 -row 0 -sticky ns
  grid $w.sh -column 0 -row 1 -sticky ew
  grid $w.f -column 1 -row 1 -sticky nsew
  grid rowconfigure $w 0 -weight 1
  grid columnconfigure $w 0 -weight 1
  set global_output(wrap) 0
  trace variable global_verbose w logverbose
  if {$global_verbose} {output "logmodus is verbose"}
}

proc logverbose {name1 name2 op} {
  global global_verbose
  if {$global_verbose} {
    output "logmodus switched to verbose" tag_orange
  } else {
    output "logmodus switched to nonverbose" tag_orange
  }
}

proc truncate_log {} {
  global global_output global_setup
  set idx [$global_output(text) index end]
  set lines [lindex [split $idx .] 0]
  if {$lines>$global_setup(loglength)} {
    $global_output(text) delete 1.0 [expr $lines-$global_setup(loglength)].0 
  }
}

proc output_face {} {
  global global_output
  $global_output(text) image create end -image warnung
}

proc output_raw {text args} {
  global global_output

  $global_output(text) config -state normal
  truncate_log
  set ttext [string trim $text]
  set atags {anfang any}
  set tags {any}
  set arglen [llength $args]
  if {$arglen>0} {
    if {$arglen==1} {set arglist [lindex $args 0]} else {set arglist $args}
    foreach i $arglist {
      lappend atags $i
      lappend tags $i
    }
  }
  set sv [lindex [$global_output(sv) get] 1]
  $global_output(text) insert end [string index $ttext 0] $atags
  $global_output(text) insert end [string range $ttext 1 end] $tags
  $global_output(text) insert end "\n"
  if {$sv==1.0} {
    $global_output(text) see end
  }
  $global_output(text) config -state disabled
}

proc output_append {text args} {
  global global_output global_flog

  set tags {any}
  set arglen [llength $args]
  if {$arglen>0} {
    if {$arglen==1} {set arglist [lindex $args 0]} else {set arglist $args}
    foreach i $arglist {
      lappend tags $i
    }
  }
  set sv [lindex [$global_output(sv) get] 1]
  $global_output(text) config -state normal
  $global_output(text) insert end $text $tags
  $global_output(text) insert end "\n"
  if {$sv==1.0} {
    $global_output(text) see end
  }
  $global_output(text) config -state disabled
}

proc output {text args} {
  global global_lasttime global_output
  set seconds [clock seconds]
  set immer 0
  if {[llength $args]>0} {
    if {"[lindex $args 0]"=="time"} {
      set immer 1
      set args [lreplace $args 0 0]
    }
  }
  if {$immer} {
    output_face
    output_raw "-- [clock format $seconds] --"
    set global_lasttime $seconds
    output_append $text $args
  } else {
    if {[expr $global_lasttime+60]<$seconds} {
      output_raw "-- [clock format $seconds] --"
      set global_lasttime $seconds
    }
    output_raw $text $args
  }
}

proc output_append_nonl {text args} {
  global global_output global_flog

  if {$global_flog!=""} {
    puts -nonewline $global_flog $text
    flush $global_flog
  }
  set tags {any}
  if {[llength $args]>0} {
    foreach i $args {
      lappend tags $i
      }
    }
  set sv [lindex [$global_output(sv) get] 1]
  $global_output(text) config -state normal
  $global_output(text) insert end $text $tags
  if {$sv==1.0} {
    $global_output(text) see end
  }
  $global_output(text) config -state disabled
}

proc output_list {text args} {
  global global_output global_flog

  $global_output(text) config -state normal
  if "$global_output(first)==0" {
    $global_output(text) insert end "\n"
    set global_output(first) 0
  }
  set tags {any}
  if {[llength $args]>0} {
    foreach i $args {
      lappend tags $i
      }
    }
  set sv [lindex [$global_output(sv) get] 1]
  foreach i $text {
    $global_output(text) insert end $i $tags
    $global_output(text) insert end "\n"
  }
  if {$sv==1.0} {
    $global_output(text) see end
  }
  $global_output(text) config -state disabled
}

proc log_see_end {} {
  global global_output
  $global_output(text) see end
  update idletasks
}
