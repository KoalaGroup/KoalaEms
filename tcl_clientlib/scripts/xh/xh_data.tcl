#
# static_arrdata(grid)
# static_arrdata(win)
# static_arrdata(cv)
# static_arrdata(sb)
# static_arrdata(list)
#
# static_arrdata_row($arr)
# static_arrdata_name($arr)
# static_arrdata_color($arr)
# static_arrdata_width($arr)
# static_arrdata_scale($arr)
# static_arrdata_vis($arr)
# static_arrdata_limit($arr)
# static_arrdata_style($arr)
#
# global_arrdata_color($arr)
# global_arrdata_width($arr)
# global_arrdata_scale($arr)
# global_arrdata_vis($arr)
# global_arrdata_limit($arr)
# global_arrdata_style($arr)
#

#-----------------------------------------------------------------------------#

proc datawin_arr_delete {arr} {
  global static_arrdata hi static_arrdata_row

  set w $static_arrdata(grid)
  set slaves [grid slaves $w -row $static_arrdata_row($arr)]
  eval destroy $slaves
}

#-----------------------------------------------------------------------------#

proc data_apply {} {
  global hi static_arrdata_name static_arrdata_color static_arrdata_width\
      static_arrdata_scale static_arrdata_vis static_arrdata_limit\
      static_arrdata_style static_arrdata
  global global_arrdata_limit
  global global_arrdata_color global_arrdata_width global_arrdata_scale
  global global_arrdata_vis global_arrdata_style

  foreach arr $static_arrdata(list) {
    $arr name $static_arrdata_name($arr)
    set limit $static_arrdata_limit($arr)
    if {[llength $limit]==1} {
      $arr limit $limit
    } else {
      $arr limit [lindex $limit 0] [lindex $limit 1]
    }
    set global_arrdata_limit($arr) $static_arrdata_limit($arr)

    $hi(win) arrconfig $arr -color $static_arrdata_color($arr)
    set global_arrdata_color($arr) $static_arrdata_color($arr)
    $hi(win) arrconfig $arr -width $static_arrdata_width($arr)
    set global_arrdata_width($arr) $static_arrdata_width($arr)
    $hi(win) arrconfig $arr -exp $static_arrdata_scale($arr)
    set global_arrdata_scale($arr) $static_arrdata_scale($arr)
    $hi(win) arrconfig $arr -enabled $static_arrdata_vis($arr)
    set global_arrdata_vis($arr) $static_arrdata_vis($arr)
    $hi(win) arrconfig $arr -style $static_arrdata_style($arr)
    set global_arrdata_style($arr) $static_arrdata_style($arr)
  }
  legend_update
}

#-----------------------------------------------------------------------------#

proc data_ok {} {
  global static_arrdata
  data_apply
  wm withdraw $static_arrdata(win)
}

#-----------------------------------------------------------------------------#

proc data_cancel {} {
  global static_arrdata
  wm withdraw $static_arrdata(win)
}

#-----------------------------------------------------------------------------#

proc datawin_delete_arr {arr} {
  global hi global_arrays

  datawin_arr_delete $arr
#  $hi(win) detacharray $arr
  $arr delete
  unset global_arrays($arr)
  legend_update
}

#-----------------------------------------------------------------------------#

proc data_apply_name {arr} {
  global static_arrdata_name

  $arr name $static_arrdata_name($arr)
  legend_update
}

#-----------------------------------------------------------------------------#

proc data_apply_color {arr} {
  global static_arrdata_color hi global_arrdata_color

  $hi(win) arrconfig $arr -color $static_arrdata_color($arr)
  set global_arrdata_color($arr) $static_arrdata_color($arr)
  legend_update
}

#-----------------------------------------------------------------------------#

proc data_apply_width {arr} {
  global static_arrdata_width hi global_arrdata_width

  $hi(win) arrconfig $arr -width $static_arrdata_width($arr)
  set global_arrdata_width($arr) $static_arrdata_width($arr)
}

#-----------------------------------------------------------------------------#

proc data_apply_scale {arr} {
  global static_arrdata_scale hi global_arrdata_scale

  $hi(win) arrconfig $arr -exp $static_arrdata_scale($arr)
  set global_arrdata_scale($arr) $static_arrdata_scale($arr)
}

#-----------------------------------------------------------------------------#

proc data_apply_vis {arr} {
  global static_arrdata_vis hi global_arrdata_vis

  $hi(win) arrconfig $arr -enabled $static_arrdata_vis($arr)
  set global_arrdata_vis($arr) $static_arrdata_vis($arr)
  legend_update
}

#-----------------------------------------------------------------------------#

proc data_apply_limit {arr} {
  global static_arrdata_limit global_arrdata_limit

  set limit $static_arrdata_limit($arr)
  if {[llength $limit]==1} {
    $arr limit $limit
  } else {
    $arr limit [lindex $limit 0] [lindex $limit 1]
  }
  set global_arrdata_limit($arr) $static_arrdata_limit($arr)
}

#-----------------------------------------------------------------------------#

proc data_apply_style {arr} {
  global static_arrdata_style hi global_arrdata_style

  $hi(win) arrconfig $arr -style $static_arrdata_style($arr)
  set global_arrdata_style($arr) $static_arrdata_style($arr)
}

#-----------------------------------------------------------------------------#

proc style_Menu {w varName arr firstValue args} {
    upvar #0 $varName var

    if ![info exists var] {
        set var $firstValue
    }
    menubutton $w -textvariable $varName -indicatoron 1 -menu $w.menu \
            -relief raised -bd 2 -highlightthickness 2 -anchor c
    menu $w.menu -tearoff 0
    $w.menu add command -label $firstValue \
            -command "set $varName $firstValue; data_apply_style $arr"
    foreach i $args {
        $w.menu add command -label $i \
          -command "set $varName $i; data_apply_style $arr"
    }
    return $w.menu
}

#-----------------------------------------------------------------------------#

proc datawin_arr_add {arr} {
  global global_arrays static_arrdata_row
  global static_arrdata hi static_arrdata_vis static_arrdata_color
  global static_arrdata_width static_arrdata_name static_arrdata_style
  global static_arrdata_scale static_arrdata_limit

  set w $static_arrdata(grid)
  set win $static_arrdata(win)
  set rows [lindex [grid size $w] 1]

  label $w.arr_$arr -text $arr -anchor w

  set static_arrdata_name($arr) [$arr name]
  entry $w.name_$arr -textvariable static_arrdata_name($arr)
  bind $w.name_$arr <Key-Return>\
      "$win.b_frame.a flash; data_apply_name $arr"
  bind $w.name_$arr <Key-KP_Enter>\
      "$win.b_frame.a flash; data_apply_name $arr"

  set static_arrdata_vis($arr) [$hi(win) arrcget $arr -enabled]
  checkbutton $w.vis_$arr -variable static_arrdata_vis($arr)\
      -command "data_apply_vis $arr"

  set static_arrdata_color($arr) [$hi(win) arrcget $arr -color]
  entry $w.color_$arr -textvariable static_arrdata_color($arr) -width 10
  bind $w.color_$arr <Key-Return>\
      "$win.b_frame.a flash; data_apply_color $arr"
  bind $w.color_$arr <Key-KP_Enter>\
      "$win.b_frame.a flash; data_apply_color $arr"

  set static_arrdata_width($arr) [$hi(win) arrcget $arr -width]
  entry $w.width_$arr -textvariable static_arrdata_width($arr) -width 5
  bind $w.width_$arr <Key-Return>\
      "$win.b_frame.a flash; data_apply_width $arr"
  bind $w.width_$arr <Key-KP_Enter>\
      "$win.b_frame.a flash; data_apply_width $arr"

  set static_arrdata_style($arr) [$hi(win) arrcget $arr -style]
  style_Menu $w.style_$arr static_arrdata_style($arr) $arr boxes lines

  set static_arrdata_scale($arr) [$hi(win) arrcget $arr -exp]
  entry $w.scale_$arr -textvariable static_arrdata_scale($arr) -width 5
  bind $w.scale_$arr <Key-Return>\
      "$win.b_frame.a flash; data_apply_scale $arr"
  bind $w.scale_$arr <Key-KP_Enter>\
      "$win.b_frame.a flash; data_apply_scale $arr"

  set static_arrdata_limit($arr) [$arr limit]
  entry $w.limit_$arr -textvariable static_arrdata_limit($arr) -width 10
  bind $w.limit_$arr <Key-Return>\
      "$win.b_frame.a flash; data_apply_limit $arr"
  bind $w.limit_$arr <Key-KP_Enter>\
      "$win.b_frame.a flash; data_apply_limit $arr"

  button $w.delete_$arr -text X -command "datawin_delete_arr $arr"
  if {$global_arrays($arr)==""} {
    $w.delete_$arr configure -state normal
  } else {
    $w.delete_$arr configure -state disabled
  }

  grid $w.arr_$arr $w.name_$arr $w.vis_$arr $w.color_$arr $w.width_$arr\
      $w.style_$arr $w.scale_$arr $w.limit_$arr $w.delete_$arr\
      -sticky w -row $rows
  set static_arrdata_row($arr) $rows
}

#-----------------------------------------------------------------------------#

proc datawin_clean_grid {} {
  global static_arrdata

  set w $static_arrdata(grid)
  set gridsize [grid size $w]
  set rows [lindex $gridsize 1]
  for {set i 1} {$i<$rows} {incr i} {
    set slaves [grid slaves $w -row $i]
    eval destroy $slaves
  }
}

#-----------------------------------------------------------------------------#

proc datawin_fill_grid {} {
  global hi static_arrdata

  set static_arrdata(list) [$hi(win) arrays]
  foreach arr $static_arrdata(list) {
    datawin_arr_add $arr
  }
}

#-----------------------------------------------------------------------------#

proc data_cancel {window} {
  global static_arrdata

#  datawin_clean_grid
  wm withdraw $window
}

#-----------------------------------------------------------------------------#

proc data_truncate {} {
  global hi

  foreach arr [$hi(win) arrays] {
    $arr truncate
  }
}

#-----------------------------------------------------------------------------#

proc scale_canvas {} {
  global static_arrdata

  set width [winfo reqwidth $static_arrdata(grid)]
  set height [winfo reqheight $static_arrdata(grid)]
  $static_arrdata(cv) configure -scrollregion "0 0 $width $height"
  set screenheight [winfo screenheight .xh_data]
  if {$height>$screenheight-100} {
    set height [expr $screenheight-100]
  }
  $static_arrdata(cv) configure -width $width -height $height
}

#-----------------------------------------------------------------------------#

proc data_win_open {} {
  global win_pos static_arrdata

  if {![winfo exists $static_arrdata(win)]} {create_data_win}
  datawin_clean_grid
  datawin_fill_grid
  if $win_pos(data) then {wm positionfrom $static_arrdata(win) user}
  wm deiconify $static_arrdata(win)
  update idletasks
  scale_canvas
  set win_pos(data) 1
  }

#-----------------------------------------------------------------------------#

proc create_data_win {} {
  global global_setup win_pos static_arrdata

  set win_pos(data) 0
  set self .xh_data
  set static_arrdata(win) $self
  toplevel $self
  wm withdraw $self
  wm group $self .
  wm title $self "[winfo name .] data"
  wm maxsize $self 10000 10000

  #frame $self.headframe -relief raised -borderwidth 1
  frame $self.gridframe -relief raised -borderwidth 1
  frame $self.b_frame -relief raised -borderwidth 1

# gridframe
  set static_arrdata(cv) [canvas $self.gridframe.cv]
  set static_arrdata(sb) [scrollbar $self.gridframe.sb\
      -command "$static_arrdata(cv) yview"]
  $static_arrdata(cv) configure -yscrollcommand "$static_arrdata(sb) set"
  set static_arrdata(grid) [frame $self.gridframe.cv.grid -relief raised\
      -borderwidth 1]
#  set static_arrdata(grid) $self.gridframe
  label $self.gridframe.cv.grid.arr -text "Array" -anchor w
  label $self.gridframe.cv.grid.name -text "Name" -anchor w
  label $self.gridframe.cv.grid.visible -text "Vis." -anchor w
  label $self.gridframe.cv.grid.color -text "Color" -anchor w
  label $self.gridframe.cv.grid.thickness -text "Thick." -anchor w
  label $self.gridframe.cv.grid.style -text "Style" -anchor w
  label $self.gridframe.cv.grid.scale -text "Scale" -anchor w
  label $self.gridframe.cv.grid.sizelimit -text "Sizelimit" -anchor w
  label $self.gridframe.cv.grid.delete -text "Delete" -anchor w
  grid $self.gridframe.cv.grid.arr $self.gridframe.cv.grid.name\
      $self.gridframe.cv.grid.visible $self.gridframe.cv.grid.color\
      $self.gridframe.cv.grid.thickness $self.gridframe.cv.grid.style\
      $self.gridframe.cv.grid.scale $self.gridframe.cv.grid.sizelimit\
      $self.gridframe.cv.grid.delete \
      -sticky w
  $self.gridframe.cv create window 0 0 -window $self.gridframe.cv.grid\
      -anchor nw
  pack $self.gridframe.cv -side left -expand 1 -fill both
  pack $self.gridframe.sb -side left -fill both
  
# buttonframe
  button $self.b_frame.ok -text OK -command "data_ok"
  button $self.b_frame.a -text Apply -command "data_apply"
  button $self.b_frame.c -text Cancel -command "data_cancel $self"
  button $self.b_frame.t -text {Truncate Files}\
    -command "$self.b_frame.t flash; data_truncate"
  pack $self.b_frame.ok $self.b_frame.a $self.b_frame.c\
      -side left -fill both -padx 2m -pady 2m
  pack $self.b_frame.t -side right -fill both -padx 2m -pady 2m
  pack $self.b_frame -side bottom -fill x
  pack $self.gridframe -side top -fill both -expand 1
}

#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
