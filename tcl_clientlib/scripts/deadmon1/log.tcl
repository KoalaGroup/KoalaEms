# $Id: log.tcl,v 1.1 2000/07/15 21:37:01 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_setup(loglength)
#
# window global_output(text)
# boolean global_output(wrap)
# window global_output(sv)
#

namespace eval ::LOG {

# window (logframe)
variable text
# boolean
variable wrap
# window (scrollbar)
variable sv
# integer
variable lasttime [clock seconds]

proc wrap {win color} {
  variable wrap
  variable text

  if $wrap {
    set wrap 0
    $text config -wrap none
    $win config -background $color
  } else {
    set wrap 1
    $text config -wrap word
    $win config -background black
  }
}

proc create_images {} {
  image create bitmap warnung -foreground red -data {
#define info_full_width 8
#define info_full_height 21
static unsigned char info_full_bits[] = {
   0x3c, 0x3e, 0x3e, 0x3e, 0x1c, 0x00, 0x00, 0x3f, 0x3f, 0x3e, 0x3c, 0x3c,
   0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0xff, 0xff, 0xff};
}
}

proc create {w} {
  global global_setup
  global global_verbose
  variable wrap
  variable text
  variable sv

  create_images
  frame $w -relief flat -borderwidth 0
  set text [text $w.t -relief ridge -borderwidth 3 \
      -wrap none \
      -state disabled \
      -yscrollcommand "$w.sv set" \
      -xscrollcommand "$w.sh set" \
      -font -*-courier-medium-r-*-*-12-*-*-*-*-*-*-* \
      -height 20]
  $text tag configure anfang -spacing1 2m
  $text tag configure any -lmargin2 5m
  $text tag configure tag_red -foreground red
  $text tag configure tag_green -foreground #007000
  $text tag configure tag_blue -foreground blue
  $text tag configure tag_orange -foreground #fe6800
  $text tag configure tag_yellow -foreground #ece800
  $text tag configure tag_under -underline 1
  $text tag configure tag_strike -overstrike 1
  set sv [scrollbar $w.sv -width 10 -command "$w.t yview"]
  scrollbar $w.sh -width 10 -orient horizontal -command "$w.t xview"
  frame $w.f -relief flat -borderwidth 0
  bind $w.f <1> [namespace code "LOG::wrap $w.f [$w.f cget -background]"]
  grid $w.t -column 0 -row 0 -sticky nsew
  grid $w.sv -column 1 -row 0 -sticky ns
  grid $w.sh -column 0 -row 1 -sticky ew
  grid $w.f -column 1 -row 1 -sticky nsew
  grid rowconfigure $w 0 -weight 1
  grid columnconfigure $w 0 -weight 1
  set wrap 0
  trace variable global_verbose w logverbose
  if {$global_verbose} {output "logmodus is verbose"}
}

proc verbose {name1 name2 op} {
  global global_verbose
  if {$global_verbose} {
    output "logmodus switched to verbose" tag_orange
  } else {
    output "logmodus switched to nonverbose" tag_orange
  }
}

proc truncate_log {} {
  global global_setup
  variable text

  set idx [$text index end]
  set lines [lindex [split $idx .] 0]
  if {$lines>$global_setup(loglength)} {
    $text delete 1.0 [expr $lines-$global_setup(loglength)].0 
  }
}

proc warning {} {
  variable text

  $text image create end -image warnung
}

proc put_raw {line args} {
  variable text
  variable sv

  $text config -state normal
  truncate_log
  set tline [string trim $line]
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
  set lsv [lindex [$sv get] 1]
  $text insert end [string index $tline 0] $atags
  $text insert end [string range $tline 1 end] $tags
  $text insert end "\n"
  if {$lsv==1.0} {
    $text see end
  }
  $text config -state disabled
}

proc append {line args} {
  variable text
  variable sv

  set tags {any}
  set arglen [llength $args]
  if {$arglen>0} {
    if {$arglen==1} {set arglist [lindex $args 0]} else {set arglist $args}
    foreach i $arglist {
      lappend tags $i
    }
  }
  set lsv [lindex [$sv get] 1]
  $text config -state normal
  $text insert end $line $tags
  $text insert end "\n"
  if {$lsv==1.0} {
    $text see end
  }
  $text config -state disabled
}

proc put {line args} {
  variable text
  variable lasttime

  set seconds [clock seconds]
  set immer 0
  if {[llength $args]>0} {
    if {"[lindex $args 0]"=="time"} {
      set immer 1
      set args [lreplace $args 0 0]
    }
  }
  if {$immer} {
    warning
    put_raw "-- [clock format $seconds] --"
    set lasttime $seconds
    LOG::append $line $args
  } else {
    if {[expr $lasttime+60]<$seconds} {
      put_raw "-- [clock format $seconds] --"
      set lasttime $seconds
    }
    put_raw $line $args
  }
}

proc append_nonl {line args} {
  variable text
  variable sv

  set tags {any}
  if {[llength $args]>0} {
    foreach i $args {
      lappend tags $i
      }
    }
  set lsv [lindex [$sv get] 1]
  $text config -state normal
  $text insert end $line $tags
  if {$lsv==1.0} {
    $text see end
  }
  $text config -state disabled
}

proc put_list {lines args} {
  variable text
  variable sv

  $text config -state normal
  set tags {any}
  if {[llength $args]>0} {
    foreach i $args {
      lappend tags $i
      }
    }
  set lsv [lindex [$sv get] 1]
  foreach i $lines {
    $text insert end $i $tags
    $text insert end "\n"
  }
  if {$lsv==1.0} {
    $text see end
  }
  $text config -state disabled
}

proc log_see_end {} {
  variable text

  $text see end
  update idletasks
}

}
