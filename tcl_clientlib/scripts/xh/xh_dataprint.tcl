#
# static_dataprint(t0_i)
# static_dataprint(t1_i)
# static_dataprint(y0)
# static_dataprint(y1)
# static_dataprint(xtics)
# static_dataprint(xticunit)
# static_dataprint(xlabs)
# static_dataprint(xlabunit)
# static_dataprint(xform)
# static_dataprint(ytics)
# static_dataprint(ylabs)
# static_dataprint(width)
# static_dataprint(height)
# static_dataprint(rotation)
# static_dataprint(eps)
# static_dataprint(pf)
# static_dataprint(color)
# static_dataprint(dash)
# static_dataprint(arrs)
# static_dataprint_enable($arr)
# static_dataprint_color($arr)
# static_dataprint_dash($arr)
# global_dataprint_enable($arr)
# global_dataprint_color($arr)
# global_dataprint_dash($arr)
#
#-----------------------------------------------------------------------------#

proc dataprint_setvar {name default} {
  global global_setup static_dataprint

  if [info exists global_setup(print_$name)] {
    set static_dataprint($name) $global_setup(print_$name)
  } else {
    set static_dataprint($name) $default 
  }
}

#-----------------------------------------------------------------------------#

proc dataprint_savevar {name} {
  global global_setup static_dataprint

  set global_setup(print_$name) $static_dataprint($name)
}

#-----------------------------------------------------------------------------#

proc dataprint_conv_times {} {
  global static_dataprint

  set from $static_dataprint(from_hour)
  append from ":"
  append from $static_dataprint(from_minute)
  append from " "
  if [regexp {^[0-9]+$} $static_dataprint(from_month)] {
    append from $static_dataprint(from_month)
    append from "/"
    append from $static_dataprint(from_day)
  } else {
    append from $static_dataprint(from_day)
    append from " "
    append from $static_dataprint(from_month)
  }
  set static_dataprint(t0_i) [clock scan $from]

  set to $static_dataprint(to_hour)
  append to ":"
  append to $static_dataprint(to_minute)
  append to " "
  if [regexp {^[0-9]+$} $static_dataprint(to_month)] {
    append to $static_dataprint(to_month)
    append to "/"
    append to $static_dataprint(to_day)
  } else {
    append to $static_dataprint(to_day)
    append to " "
    append to $static_dataprint(to_month)
  }
  set static_dataprint(t1_i) [clock scan $to]
}

#-----------------------------------------------------------------------------#

proc dataprint_reset_axes {} {
  global static_dataprint hi

  set a [lindex [split [$hi(win) xmin] .] 0]
  set b [lindex [split [$hi(win) xmax] .] 0]
  set static_dataprint(t0_i) $a
  set static_dataprint(t1_i) $b
  
  set static_dataprint(from_day) [clock format $a -format {%d}]
  set static_dataprint(from_month) [clock format $a -format {%b}]
  set static_dataprint(from_hour) [clock format $a -format {%H}]
  set static_dataprint(from_minute) [clock format $a -format {%M}]

  set static_dataprint(to_day) [clock format $b -format {%d}]
  set static_dataprint(to_month) [clock format $b -format {%b}]
  set static_dataprint(to_hour) [clock format $b -format {%H}]
  set static_dataprint(to_minute) [clock format $b -format {%M}]

  set static_dataprint(y0) 0
  set static_dataprint(y1) [$hi(win) ymax]
}

#-----------------------------------------------------------------------------#

proc dataprint_reset {} {
  global global_setup static_dataprint hi
  global static_dataprint_enable static_dataprint_color static_dataprint_dash

  dataprint_reset_axes

  dataprint_setvar xtics 1
  dataprint_setvar xticunit minutes
  dataprint_setvar xlabs 10
  dataprint_setvar xlabunit minutes
  dataprint_setvar xform {%a %H:%M}
  dataprint_setvar ytics 10
  dataprint_setvar ylabs 20
  dataprint_setvar width 800
  dataprint_setvar height 400
  dataprint_setvar rotation 90
  dataprint_setvar color 0
  dataprint_setvar dash 0
  dataprint_setvar eps ps
  dataprint_setvar pf print
  dataprint_setvar fname ""
  dataprint_setvar lpr "lpr"

  if {$static_dataprint(pf) == "write"} {
    $static_dataprint(printframe_x).l configure -text "Filename :"
    $static_dataprint(printframe_x).e configure\
        -textvariable static_dataprint(fname)
  } else {
    $static_dataprint(printframe_x).l configure -text "Command :"
    $static_dataprint(printframe_x).e configure\
        -textvariable static_dataprint(lpr)
  }
  foreach arr $static_dataprint(arrs) {
    if [info exists global_dataprint_enable($arr)] {
      set static_dataprint_enable($arr) $global_dataprint_enable($arr)
    } else {
      set static_dataprint_enable($arr) [$hi(win) arrcget $arr -enabled]
    }
    if [info exists global_dataprint_color($arr)] {
      set static_dataprint_color($arr) $global_dataprint_color($arr)
    } else {
      set static_dataprint_color($arr) [$hi(win) arrcget $arr -color]
    }
    if [info exists global_dataprint_dash($arr)] {
      set static_dataprint_dash($arr) $global_dataprint_dash($arr)
    } else {
      set static_dataprint_dash($arr) {}
    }
  }
}

#-----------------------------------------------------------------------------#

proc dataprint_reset_default {} {
  global global_setup static_dataprint hi
  global static_dataprint_enable static_dataprint_color static_dataprint_dash
  
  dataprint_reset_axes

  set static_dataprint(xtics) 1
  set static_dataprint(xticunit) minutes
  set static_dataprint(xlabs) 10
  set static_dataprint(xlabunit) minutes
  set static_dataprint(xform) {%a %H:%M}
  set static_dataprint(ytics) 10
  set static_dataprint(ylabs) 20
  set static_dataprint(width) 800
  set static_dataprint(height) 400
  set static_dataprint(rotation) 90
  set static_dataprint(color) 0
  set static_dataprint(dash) 0
  set static_dataprint(eps) ps
  set static_dataprint(pf) print
  set static_dataprint(fname ""
  set static_dataprint(lpr) "lpr"

  $static_dataprint(printframe_x).l configure -text "Command :"
  $static_dataprint(printframe_x).e configure\
      -textvariable static_dataprint(lpr)

  foreach arr $static_dataprint(arrs) {
    set static_dataprint_enable($arr) [$hi(win) arrcget $arr -enabled]
    set static_dataprint_color($arr) [$hi(win) arrcget $arr -color]
    set static_dataprint_dash($arr) {}
  }
}

#-----------------------------------------------------------------------------#

proc dataprint_eps {} {
  global static_dataprint
  
  if {$static_dataprint(eps)=="eps"} {
    set static_dataprint(pf) write
    $static_dataprint(printframe_x).l configure -text "Filename :"
    $static_dataprint(printframe_x).e configure\
        -textvariable static_dataprint(fname)
  }
}

#-----------------------------------------------------------------------------#

proc dataprint_pf {} {
  global static_dataprint
  
  if {$static_dataprint(pf)=="print"} {
    set static_dataprint(eps) ps
    $static_dataprint(printframe_x).l configure -text "Command :"
    $static_dataprint(printframe_x).e configure\
        -textvariable static_dataprint(lpr)
  } else {
    $static_dataprint(printframe_x).l configure -text "Filename :"
    $static_dataprint(printframe_x).e configure\
        -textvariable static_dataprint(fname)
  }
}

#-----------------------------------------------------------------------------#

proc dataprint_apply {} {
  global global_setup static_dataprint
  global global_dataprint_enable static_dataprint_enable
  global global_dataprint_color static_dataprint_color
  global global_dataprint_dash static_dataprint_dash
  
  dataprint_conv_times
  dataprint_savevar width
  dataprint_savevar height
  dataprint_savevar rotation
  dataprint_savevar pf
  dataprint_savevar color
  dataprint_savevar dash
  dataprint_savevar eps
  dataprint_savevar lpr
  dataprint_savevar fname
  dataprint_savevar xtics
  dataprint_savevar xticunit
  dataprint_savevar xlabs
  dataprint_savevar xlabunit
  dataprint_savevar xform
  dataprint_savevar ytics
  dataprint_savevar ylabs

  foreach arr $static_dataprint(arrs) {
    set global_dataprint_enable($arr) $static_dataprint_enable($arr)
    set global_dataprint_color($arr) $static_dataprint_color($arr)
    set global_dataprint_dash($arr) $static_dataprint_dash($arr)
  }
  ladd global_setup_extra global_dataprint_enable
  ladd global_setup_extra global_dataprint_color
  ladd global_setup_extra global_dataprint_dash

  dataprint_psprint
}

#-----------------------------------------------------------------------------#

proc dataprint_ok {window} {
  dataprint_apply
  wm withdraw $window
}

#-----------------------------------------------------------------------------#

proc dataprint_cancel {window} {
  wm withdraw $window
}

#-----------------------------------------------------------------------------#

proc dataprint_scale_canvas {} {
  global static_dataprint

  set f $static_dataprint(arrframe)
  set width [winfo reqwidth $f.cv.grid]
  set height [winfo reqheight $f.cv.grid]
  $f.cv configure -scrollregion "0 0 $width $height"
  set screenheight [winfo screenheight .xh_dataprint]
  if {$height>$screenheight-100} {
    set height [expr $screenheight-100]
  }
  $f.cv configure -width $width -height $height
}

#-----------------------------------------------------------------------------#

proc dataprint_fill_arrframe {} {
  global static_dataprint hi
  global global_dataprint_enable static_dataprint_enable
  global global_dataprint_color static_dataprint_color
  global global_dataprint_dash static_dataprint_dash

  set f $static_dataprint(arrframe)
  if [winfo exists $f] {
    set wlist [winfo children $f]
    foreach i $wlist {
      destroy $i
    }
  }
  canvas $f.cv
  scrollbar $f.sb -command "$f.cv yview"
  $f.cv configure -yscrollcommand "$f.sb set"
  frame $f.cv.grid -relief raised -borderwidth 1

  entry $f.e
  set efont [$f.e cget -font]
  destroy $f.e
  set c $f.cv.grid
  label $c.arr -text Array -anchor w
  label $c.name -text Name -anchor w
  label $c.check -text Print -anchor w
  label $c.color -text Color -anchor w
  label $c.dash -text Dash -anchor w
  grid $c.arr $c.name $c.check $c.color $c.dash -sticky w 
  foreach arr $static_dataprint(arrs) {
    label $c.arr_$arr -text $arr -anchor w -font $efont
    label $c.name_$arr -text [$arr name] -anchor w -font $efont
    if [info exists global_dataprint_enable($arr)] {
      set static_dataprint_enable($arr) $global_dataprint_enable($arr)
    } else {
      set static_dataprint_enable($arr) [$hi(win) arrcget $arr -enabled]
    }
    checkbutton $c.check_$arr -variable static_dataprint_enable($arr)
    if [info exists global_dataprint_color($arr)] {
      set static_dataprint_color($arr) $global_dataprint_color($arr)
    } else {
      set static_dataprint_color($arr) [$hi(win) arrcget $arr -color]
    }
    entry $c.color_$arr -textvariable static_dataprint_color($arr)\
        -width 10
    if [info exists global_dataprint_dash($arr)] {
      set static_dataprint_dash($arr) $global_dataprint_dash($arr)
    } else {
      set static_dataprint_dash($arr) {}
    }
    entry $c.dash_$arr -textvariable static_dataprint_dash($arr)\
        -width 10
    grid $c.arr_$arr $c.name_$arr $c.check_$arr $c.color_$arr $c.dash_$arr\
        -sticky ew 
  }
  $f.cv create window 0 0 -window $f.cv.grid -anchor nw
  pack $f.cv -side left -expand 1 -fill both
  pack $f.sb -side left -fill both
}

#-----------------------------------------------------------------------------#

proc dataprint_win_open {} {
  global win_pos static_dataprint hi

  if [winfo exists .xh_dataprint] {destroy .xh_dataprint}
  if {![winfo exists .xh_dataprint]} {create_dataprint_win}
  if $win_pos(dataprint) then {wm positionfrom .xh_dataprint user}
  set static_dataprint(arrs) [$hi(win) arrays]
  dataprint_reset
  dataprint_fill_arrframe
  wm deiconify .xh_dataprint
  update idletasks
  dataprint_scale_canvas
  set win_pos(dataprint) 1
  }

#-----------------------------------------------------------------------------#

proc dataprint_rangeframe {frame} {
  global static_dataprint


# xy
  label $frame.lfrom -text "from :" -anchor w
  label $frame.lto   -text "to :" -anchor w
  label $frame.lymax -text "ymax :" -anchor w
  entry $frame.ymax -textvariable static_dataprint(y1)

  entry $frame.fday    -textvariable static_dataprint(from_day) -width 0
  entry $frame.fmonth  -textvariable static_dataprint(from_month) -width 0
  entry $frame.fhour   -textvariable static_dataprint(from_hour) -width 0
  entry $frame.fminute -textvariable static_dataprint(from_minute) -width 0

  entry $frame.tday    -textvariable static_dataprint(to_day) -width 0
  entry $frame.tmonth  -textvariable static_dataprint(to_month) -width 0
  entry $frame.thour   -textvariable static_dataprint(to_hour) -width 0
  entry $frame.tminute -textvariable static_dataprint(to_minute) -width 0
  set font [$frame.fday cget -font]
  label $frame.day    -font $font -text "(day" -width 0
  label $frame.month  -font $font -text month -width 0
  label $frame.hour   -font $font -text hour -width 0
  label $frame.minute -font $font -text "min)" -width 0

  set row 0
  grid $frame.day -column 1 -row $row -sticky ew
  grid $frame.month -column 2 -row $row -sticky ew
  grid $frame.hour -column 3 -row $row -sticky ew
  grid $frame.minute -column 4 -row $row -sticky ew
  grid columnconfigure $frame 0 -weight 0
  grid columnconfigure $frame 1 -weight 1
  grid columnconfigure $frame 2 -weight 1
  grid columnconfigure $frame 3 -weight 1
  grid columnconfigure $frame 4 -weight 1
  incr row 
  grid $frame.lfrom $frame.fday $frame.fmonth $frame.fhour $frame.fminute\
      -row $row -sticky ew
  incr row 
  grid $frame.lto $frame.tday $frame.tmonth $frame.thour $frame.tminute\
      -row $row -sticky ew
  incr row 
  grid $frame.lymax $frame.ymax - - - -sticky ew -row $row
  incr row 

# tics
  label $frame.lxtic -text "X tics :" -anchor w
  label $frame.lxlab -text "X labels :" -anchor w
  label $frame.lxform -text "Format :" -anchor w
  label $frame.lytic -text "Y tics :" -anchor w
  label $frame.lylab -text "Y labels :" -anchor w
  entry $frame.extic -textvariable static_dataprint(xtics) -width 0
  tk_optionMenu $frame.mxtic static_dataprint(xticunit)\
      seconds minutes hours days
  entry $frame.exlab -textvariable static_dataprint(xlabs) -width 0
  tk_optionMenu $frame.mxlab static_dataprint(xlabunit)\
      seconds minutes hours days
  entry $frame.exform -textvariable static_dataprint(xform)
  entry $frame.eytic -textvariable static_dataprint(ytics)
  entry $frame.eylab -textvariable static_dataprint(ylabs)
  grid rowconfigure $frame $row -minsize 2m
  incr row 
  grid $frame.lxtic $frame.extic - $frame.mxtic - -row $row -sticky ew 
  incr row 
  grid $frame.lxlab $frame.exlab - $frame.mxlab - -row $row -sticky ew
  incr row 
  grid $frame.lxform $frame.exform - - - -row $row -sticky ew
  incr row 
  grid $frame.lytic $frame.eytic - - - -row $row -sticky ew
  incr row 
  grid $frame.lylab $frame.eylab - - - -row $row -sticky ew
  incr row 

# paper
  label $frame.lp -text "Paper:" -anchor w
  label $frame.lpw -text "width :" -anchor w
  label $frame.lph -text "height :" -anchor w
  label $frame.lpr -text "rotation :" -anchor w
  entry $frame.epw -textvariable static_dataprint(width) -width 0
  entry $frame.eph -textvariable static_dataprint(height) -width 0
  tk_optionMenu $frame.mpr static_dataprint(rotation)\
      {0} {90}
  grid rowconfigure $frame $row -minsize 2m
  incr row 
  grid $frame.lp -sticky ew -row $row
  incr row 
  grid $frame.lpw $frame.epw - - - -row $row -sticky ew
  incr row 
  grid $frame.lph $frame.eph - - - -row $row -sticky ew
  incr row 
  grid $frame.lpr $frame.mpr - - - -row $row -sticky ew
}

#-----------------------------------------------------------------------------#

proc dataprint_printframe {frame_a frame_b frame_c frame_d} {
  global static_dataprint

# printframe_a
  radiobutton $frame_a.ps \
      -text "Postscript" -variable static_dataprint(eps)\
      -value ps -command dataprint_eps -anchor w
  radiobutton $frame_a.eps \
      -text "Encapsulated Postscript" -variable static_dataprint(eps)\
      -value eps -command dataprint_eps -anchor w
  pack $frame_a.ps $frame_a.eps\
      -side top -padx 1m -fill x -expand 1

# printframe_b
  checkbutton $frame_b.color \
      -text "Enable Color" -variable static_dataprint(color)\
      -anchor w
  checkbutton $frame_b.dash \
      -text "Enable Dash" -variable static_dataprint(dash)\
      -anchor w
  pack $frame_b.color $frame_b.dash\
      -side top -padx 1m -fill x -expand 1

# printframe_c
  radiobutton $frame_c.print \
      -text "Send to Printer" -variable static_dataprint(pf)\
      -value print -command dataprint_pf -anchor w
  radiobutton $frame_c.write \
      -text "Write File" -variable static_dataprint(pf)\
      -value write -command dataprint_pf -anchor w
  pack $frame_c.print $frame_c.write\
      -side top -padx 1m -fill x -expand 1

# printframe_d
  set static_dataprint(printframe_x) $frame_d
  label $frame_d.l -anchor w
  entry $frame_d.e -width 20
  pack $frame_d.l $frame_d.e\
      -side top -padx 1m -fill x -expand 1
}

#-----------------------------------------------------------------------------#

proc create_dataprint_win {} {
  global global_setup win_pos static_dataprint

  set win_pos(dataprint) 0
  set self .xh_dataprint
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] print data"
  wm maxsize $self 10000 10000

  frame $self.l -relief raised -borderwidth 1
  frame $self.r -relief raised -borderwidth 1
  frame $self.b -relief raised -borderwidth 1
  set static_dataprint(arrframe) $self.l

  frame $self.r.rangeframe -relief raised -borderwidth 1
  dataprint_rangeframe $self.r.rangeframe

  frame $self.r.printframe_a -relief raised -borderwidth 1
  frame $self.r.printframe_b -relief raised -borderwidth 1
  frame $self.r.printframe_c -relief raised -borderwidth 1
  frame $self.r.printframe_d -relief raised -borderwidth 1
  dataprint_printframe $self.r.printframe_a $self.r.printframe_b\
      $self.r.printframe_c $self.r.printframe_c

# buttonframe
  button $self.b.ok -text OK -command "dataprint_ok $self"
  button $self.b.a -text Apply -command "dataprint_apply"
  button $self.b.r -text Reset -command "dataprint_reset"
  button $self.b.c -text Cancel -command "dataprint_cancel $self"
  button $self.b.ra -text "Reset Axes" -command "dataprint_reset_axes"
  button $self.b.d -text "Reset to default" -command "dataprint_reset_default"
  grid $self.b.ra - $self.b.d - -sticky ew -padx 2m -pady 2m
  grid $self.b.ok $self.b.a $self.b.r $self.b.c -sticky ew -padx 2m -pady 2m

  pack $self.r.rangeframe\
      $self.r.printframe_a $self.r.printframe_b $self.r.printframe_c\
      $self.r.printframe_d -side top -fill both -expand 1

  pack $self.l -side left -fill both -expand 1
  pack $self.b -side bottom -fill both
  pack $self.r -side top -fill both -expand 1
}

#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
