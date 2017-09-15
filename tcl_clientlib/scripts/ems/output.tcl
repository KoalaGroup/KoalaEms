#
# global vars in this file:
#
# window global_output(win)
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

proc create_output {title} {
  global global_output
  set w .output
  set global_output(win) $w
  toplevel $w -class Output -relief ridge -borderwidth 5
  wm withdraw $w
  wm title $w $title
  wm iconname $w Output
  wm geometry $w 300x300
  frame $w.ft -relief flat -borderwidth 0
  frame $w.fb -relief flat -borderwidth 0
  set global_output(text) [text $w.ft.t -relief raised -borderwidth 2\
      -width 0 -height 0\
      -wrap none\
      -state disabled\
      -yscrollcommand "$w.ft.s set"\
      -xscrollcommand "$w.fb.s set"\
      -font -*-courier-medium-r-*-*-12-*-*-*-*-*-*-*]
  $global_output(text) tag configure anfang -spacing1 2m
  $global_output(text) tag configure any -lmargin2 5m
  scrollbar $w.ft.s -width 10 -command "$w.ft.t yview"
  scrollbar $w.fb.s -width 10 -orient horizontal -command "$w.ft.t xview"
  set width [expr [$w.ft.s cget -width]+2*[$w.ft.s cget -borderwidth]\
      +2*[$w.ft.s cget -highlightthickness]]
  frame $w.fb.f -relief flat -borderwidth 0 -width $width\
      -height $width
  bind $w.fb.f <1> "output_wrap $w.fb.f [$w.fb.f cget -background]"
  pack $w.ft.t -side left -fill both -expand 1
  pack $w.ft.s -side right -fill y
  pack $w.fb.f -side right
  pack $w.fb.s -side left -fill x -expand 1
  pack $w.ft -side top -fill both -expand 1
  pack $w.fb -side bottom -fill x
  set global_output(append) 0
  set global_output(wrap) 0
}

proc output {text} {
  global global_output
  $global_output(text) config -state normal
  if "$global_output(append)" {
    set tag anfang
  } else {
    set tag {}
    set global_output(append) 1
  }
#  set ttext [string trim $text]
  set ttext $text
  $global_output(text) insert end [string index $ttext 0] "$tag any"
  $global_output(text) insert end [string range $ttext 1 end] any
  $global_output(text) insert end "\n"
  $global_output(text) see end
  $global_output(text) config -state disabled
  wm deiconify $global_output(win)
}

proc output_append {text} {
  global global_output
  $global_output(text) config -state normal
#  set ttext [string trim $text]
  set ttext $text
  $global_output(text) insert end $ttext any
  $global_output(text) insert end "\n"
  $global_output(text) see end
  $global_output(text) config -state disabled
  wm deiconify $global_output(win)
}

proc output_list {text} {
  global global_output
  $global_output(text) config -state normal
  if "$global_output(first)==0" {
    $global_output(text) insert end "\n"
    set global_output(first) 0
  }
  foreach i $text {
    $global_output(text) insert end $i
    $global_output(text) insert end "\n"
    $global_output(text) see end
  }
  $global_output(text) config -state disabled
  wm deiconify $global_output(win)
}
