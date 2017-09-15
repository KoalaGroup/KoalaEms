# $ZEL: scaler_channel_win.tcl,v 1.5 2011/01/13 20:14:35 wuestner Exp $
# copyright 1998
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_setup(num_panes)
# global_setup_names()
# global_setup_vis()
# global_setup_pane()
# global_num_channels
# static_channels_name
# static_channels_vis
# static_channels_print
# static_channels_pane
# 

#-----------------------------------------------------------------------------#

proc channels_reset {} {
  global global_setup global_num_channels
  global global_setup_names static_channels_name
  global global_setup_vis static_channels_vis
  global global_setup_print static_channels_print
  global global_setup_pane static_channels_pane

  for {set i 0} {$i<$global_num_channels} {incr i} {
    if [info exists global_setup_names($i)] {
      set static_channels_name($i) $global_setup_names($i)
    } else {
      set static_channels_name($i) Channel_$i
    }
    if [info exists global_setup_vis($i)] {
      set static_channels_vis($i) $global_setup_vis($i)
    } else {
      set static_channels_vis($i) 1
    }
    if [info exists global_setup_print($i)] {
      set static_channels_print($i) $global_setup_print($i)
    } else {
      set static_channels_print($i) 1
    }
    if [info exists global_setup_pane($i)] {
      set static_channels_pane($i) $global_setup_pane($i)
    } else {
      set static_channels_pane($i) 0
    }
  }
}

#-----------------------------------------------------------------------------#

proc channels_apply {} {
  global global_setup_extra global_num_channels
  global global_setup_names static_channels_name
  global global_setup_vis static_channels_vis
  global global_setup_print static_channels_print
  global global_setup_pane static_channels_pane
  global global_running global_xh_path

  if {$global_running(graphical_display)} {
    for {set i 0} {$i<$global_num_channels} {incr i} {
      if {[info exists global_setup_names($i)] 
          && ("$global_setup_names($i)"!="$static_channels_name($i)")} {
        if [catch {
              puts $global_xh_path "\{name scaler_$i \{$static_channels_name($i)\}\}"
              flush $global_xh_path
            } mist ] {
          output $mist
          set global_running(graphical_display) 0
        }
      }
    }
  }
  for {set i 0} {$i<$global_num_channels} {incr i} {
    set global_setup_names($i) $static_channels_name($i)
    set global_setup_vis($i) $static_channels_vis($i)
    set global_setup_print($i) $static_channels_print($i)
    set global_setup_pane($i) $static_channels_pane($i)
  }
  ladd global_setup_extra global_setup_names
  ladd global_setup_extra global_setup_vis
  ladd global_setup_extra global_setup_print
  ladd global_setup_extra global_setup_pane
  change_display
}

#-----------------------------------------------------------------------------#

proc channels_ok {} {
  global static_channels
  channels_apply
  wm withdraw $static_channels(win)
}

#-----------------------------------------------------------------------------#

proc channels_cancel {} {
  global static_channels
  channels_reset
  wm withdraw $static_channels(win)
}

#-----------------------------------------------------------------------------#

proc channelwin_chan_add {channel row} {
  global global_setup static_channels 
  global static_channels_name static_channels_vis
  global static_channels_print static_channels_pane

  set w $static_channels(grid)
  set win $static_channels(win)

  label $w.idx_$channel -text $channel -anchor e

  entry $w.name_$channel -textvariable static_channels_name($channel)

  checkbutton $w.vis_$channel -variable static_channels_vis($channel)

  checkbutton $w.print_$channel -variable static_channels_print($channel)

  grid $w.idx_$channel $w.name_$channel $w.print_$channel $w.vis_$channel -sticky e -row $row

  for {set i 0} {$i<$global_setup(num_panes)} {incr i} {
    radiobutton $w.pane_($channel)_$i -variable static_channels_pane($channel)\
        -value $i
    grid $w.pane_($channel)_$i -row $row -column [expr 4+$i]
  }
}

#-----------------------------------------------------------------------------#

proc channelwin_evrate_add {row} {
  
}

#-----------------------------------------------------------------------------#

proc channelwin_clean_grid {} {
  global static_channels

  set w $static_channels(grid)
  set gridsize [grid size $w]
  set rows [lindex $gridsize 1]
  for {set i 0} {$i<$rows} {incr i} {
    set slaves [grid slaves $w -row $i]
    eval destroy $slaves
  }
}

#-----------------------------------------------------------------------------#

proc channelwin_fill_grid {} {
  global global_setup static_channels global_num_channels

  set w $static_channels(grid)
  set win $static_channels(win)

  label $w.idx -text "Channel" -anchor w
  label $w.name -text "Name" -anchor w
  label $w.print -text "Print" -anchor w
  label $w.visible -text "Vis." -anchor w
  grid $w.idx $w.name $w.print $w.visible -row 0
  for {set i 0} {$i<$global_setup(num_panes)} {incr i} {
    label $w.pane_$i -text "Pane $i" -anchor w
    grid $w.pane_$i -row 0 -column [expr 4+$i]
  }
  channelwin_evrate_add 1
  for {set i 0} {$i<$global_num_channels} {incr i} {
    channelwin_chan_add $i [expr $i+2]
  }
}

#-----------------------------------------------------------------------------#

proc channels_cancel {window} {
  wm withdraw $window
}

#-----------------------------------------------------------------------------#

proc channel_win_scale_canvas {} {
  global static_channels

  set width [winfo reqwidth $static_channels(grid)]
  set height [winfo reqheight $static_channels(grid)]
  $static_channels(cv) configure -scrollregion "0 0 $width $height"
  set screenheight [winfo screenheight .xh_channels]
  if {$height>$screenheight-100} {
    set height [expr $screenheight-100]
  }
  $static_channels(cv) configure -width $width -height $height
}

#-----------------------------------------------------------------------------#

proc channel_win_open {} {
  global win_pos static_channels

  #if [winfo exists .xh_channels] {destroy .xh_channels}
  if {![winfo exists .xh_channels]} channel_win_create
  channels_reset
  channelwin_clean_grid
  channelwin_fill_grid
  if $win_pos(channels) then {wm positionfrom $static_channels(win) user}
  wm deiconify $static_channels(win)
  update idletasks
  channel_win_scale_canvas
  set win_pos(channels) 1
  }

#-----------------------------------------------------------------------------#

proc channel_win_create {} {
  global win_pos static_channels

  set win_pos(channels) 0
  set self .xh_channels
  set static_channels(win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] channels"
  wm maxsize $self 10000 10000

  frame $self.gridframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# gridframe
  set static_channels(cv) [canvas $self.gridframe.cv]
  set static_channels(sb) [scrollbar $self.gridframe.sb\
      -command "$static_channels(cv) yview"]
  $static_channels(cv) configure -yscrollcommand "$static_channels(sb) set"
  set static_channels(grid) [frame $self.gridframe.cv.grid -relief raised\
      -borderwidth 1]
  $self.gridframe.cv create window 0 0 -window $self.gridframe.cv.grid\
      -anchor nw

  pack $self.gridframe.cv -side left -expand 1 -fill both
  pack $self.gridframe.sb -side left -fill both
  
# buttonframe
  button $self.b_frame.ok -text OK -command "channels_ok"
  button $self.b_frame.a -text Apply -command "channels_apply"
  button $self.b_frame.c -text Cancel -command "channels_cancel $self"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m
  pack $self.b_frame -side bottom -fill x
  pack $self.gridframe -side top -fill both -expand 1
}

#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
