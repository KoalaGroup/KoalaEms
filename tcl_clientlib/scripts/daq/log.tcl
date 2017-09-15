# $ZEL: log.tcl,v 1.19 2009/04/01 16:29:58 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_setup(logfiledir)
# global_setup(loglength)
# global_setup(maxlogfiles)
# global_logfilename
# global_flog
#
# window global_output(text)
# boolean global_output(wrap)
# window global_output(sv)
#

proc open_logfile {} {
    global global_setup global_flog
    global global_last_logfilestart

    set global_flog ""
    set global_last_logfilestart [clock seconds]
    set list [glob -nocomplain $global_setup(logfiledir)/daqlog_\[0-9\]*]
    set llen [llength $list]
    if {$llen>$global_setup(maxlogfiles)} {
        output "Your logfile directory ($global_setup(logfiledir)) contains $llen files." tag_blue
        output_append "You should delete or archive the oldest files." tag_blue
    }
    if {$llen==0} {
        set name $global_setup(logfiledir)/daqlog_1
    } else {
        set nlist {}
        foreach i $list {
            regsub .*daqlog_ $i {} name
            lappend nlist $name
        }
        set slist [lsort -integer -decreasing $nlist]
        set last [lindex $slist 0]
        set next [expr $last+1]
        set name $global_setup(logfiledir)/daqlog_$next
    }

    if [catch {set global_flog [open $name {RDWR CREAT} 0640]} mist] {
        output $mist tag_red
        set global_flog ""
    } else {
        output "logfile \"$name\" open"
    }
}

proc close_log {} {
  global global_flog
  if {$global_flog!=""} {
    output_append "log closed" time
    close $global_flog
  }
  set global_flog ""
}

proc check_new_logfile {} {
    global global_last_logfilestart
    set range 86400 ;# 1 day

    set now [clock seconds]
    if {$now/$range!=$global_last_logfilestart/$range} {
        output "starting a new logfile"
        close_log
        open_logfile
    }
}

proc truncate_log {length} {
  global global_output
  #puts "called truncate_log $length"
  if {$length==0} {
    $global_output(text) configure -state normal
    $global_output(text) delete 1.0 end
    $global_output(text) configure -state disabled
  } else {
    set idx [$global_output(text) index end]
    set lines [lindex [split $idx .] 0]
    if {$lines>$length} {
      $global_output(text) configure -state normal
      $global_output(text) delete 1.0 [expr $lines-$length].0 
      $global_output(text) configure -state disabled
    }
  }
}

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
  image create photo face -palette 100
  face read $EMSTCL_HOME/daq/doomboss.gif
}

proc create_log {w} {
  global global_output global_setup
  global global_logfilename global_verbose

  create_images
  frame $w -relief flat -borderwidth 0
  set global_output(text) [text $w.t -relief ridge -borderwidth 3\
      -wrap none\
      -state disabled\
      -yscrollcommand "$w.sv set"\
      -xscrollcommand "$w.sh set"]
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
  frame $w.f -relief flat -borderwidth 1
  bind $w.f <1> "output_wrap $w.f [$w.f cget -background]"
  grid $w.t -column 0 -row 0 -sticky nsew
  grid $w.sv -column 1 -row 0 -sticky ns
  grid $w.sh -column 0 -row 1 -sticky ew
  grid $w.f -column 1 -row 1 -sticky nsew
  grid rowconfigure $w 0 -weight 1
  grid columnconfigure $w 0 -weight 1
  set global_output(wrap) 1
  $global_output(text) config -wrap word
  $w.f config -background black

  open_logfile

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

proc output_face {} {
  global global_output
  $global_output(text) image create end -image face
#  $global_output(text) image create end -image warnung
}

# ich will folgendes:
# output "text" append color:red attr:underlined
# output "text" color:#ff00ff attr:dashed

proc make_tags {text tags} {
    set current_tags [$text tag names]
    foreach tag $tags {
        if {[regsub (color:)(.*) $tag {\2} color]} {
            if {[lsearch -exact $current_tags $tag]<0} {
                catch {$text tag configure $tag -foreground $color}
            }
        }
    }
}

proc output_raw {text args} {
    global global_output global_flog
    global global_setup

    truncate_log $global_setup(loglength)
    $global_output(text) config -state normal
    set ttext [string trim $text]
    if {$global_flog!=""} {
        puts $global_flog ""
        puts $global_flog $ttext
        flush $global_flog
    }
    make_tags $global_output(text) $args
    set atags [concat {anfang any} $args]
    set tags [concat {any} $args]

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

    if {$global_flog!=""} {
        puts $global_flog $text
        flush $global_flog
    }
    make_tags $global_output(text) $args
    set tags [concat {any} $args]

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
    global clockformat

    set seconds [clock seconds]
    set immer 0
    if {[llength $args]>0} {
        if {"[lindex $args 0]"=="time"} {
            set immer 1
            set args [lreplace $args 0 0]
        }
    }
    if {$immer} {
        check_new_logfile
        output_face
        output_raw "-- [clock format $seconds -format $clockformat] --"
        set global_lasttime $seconds
        output_append $text $args
        log_see_end
    } else {
        if {[expr $global_lasttime+60]<$seconds} {
            output_raw "-- [clock format $seconds -format $clockformat] --"
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
  make_tags $global_output(text) $args
  set tags [concat {any} $args]

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
  if {$global_flog!=""} {
    foreach i $text {
      puts $global_flog $i
    }
    flush $global_flog
  }
  make_tags $global_output(text) $args
  set tags [concat {any} $args]

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
