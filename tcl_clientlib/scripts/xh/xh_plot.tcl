# $ZEL: xh_plot.tcl,v 1.4 2004/11/18 12:07:46 wuestner Exp $
# copyright 1998 ... 2004
#   Peter Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

#
# global vars in this file:
#
# global_setup(x_axis_diff)
# global_setup(y_axis_max)
#
# cv(win)
# cv(font)
# cv(ticl)
# cv(width)
# cv(height)
# cv(leftdist)
# cv(rightdist)
# cv(topdist)
# cv(bottomdist)
# cv(tw_) : Breite einer Zahl mit 5 Ziffern
# cv(th_) : Hoehe einer Zahl
# cv(ttw) : Breite einer Uhrzeit
# cv(space)
# cross(x)
# cross(y)
# cross(active)
# hi(win)
# hi_win
# hi(x0)
# hi(x1)
# hi(y0)
# hi(y1)
#

proc delnull {v} {
  set w [expr $v]
  if {[string first . $w]!=-1} {
    set w [string trimright [string trimright $w 0] .]
  }
  return $w
}

proc make_x_scale {min max} {
  global cv hi hi_win

#puts "make_x_scale $min $max"
  $cv(win) delete Xlabel
  set ii [split $min .]
  set imin [lindex $ii 0]
  if {[lindex $ii 1] != 0} {incr imin}
  set imax [lindex [split $max .] 0]
  set idiff [expr $imax-$imin]
  set width [$hi_win cget -width]
#puts "imin=$imin imax=$imax idiff=$idiff width=$width"
  set spix [expr $width/$idiff]
  set mpix [expr $width*60/$idiff]
  set hpix [expr $width*3600/$idiff]
#puts "spix=$spix mpix=$mpix hpix=$hpix"
  set platz [expr 1*$cv(ttw)]
#puts "platz=$platz"
  set yo [expr $hi(y0)-$cv(ticl)]
  set yso [expr $hi(y0)-$cv(sticl)]
  set yu [expr $hi(y1)+$cv(ticl)]
  set ysu [expr $hi(y1)+$cv(sticl)]
#  puts "spix=$spix mpix=$mpix hpix=$hpix"
  set ypos [expr $hi(y1)+$cv(ticl)+$cv(space)]

  set x [$hi_win transevx $imin]
  $cv(win) create line\
    $x $yso\
    $x $ysu\
    -tags Xlabel\
    -width 0
  $cv(win) create text $x $ypos\
    -anchor n\
    -font $cv(font)\
    -tags Xlabel\
    -text [clock format $imin -format %H:%M'%S]

  set x [$hi_win transevx $imax]
  $cv(win) create line\
    $x $yso\
    $x $ysu\
    -tags Xlabel\
    -width 0
  $cv(win) create text $x $ypos\
    -anchor n\
    -font $cv(font)\
    -tags Xlabel\
    -text [clock format $imax -format %H:%M'%S]

  set leftx [expr $hi(x0)+$cv(ttw)]
  set rightx [expr $hi(x1)-$cv(ttw)]
  if {$spix>$platz} {
#    puts "1s 1s"
    set ti 1; set li 1; set fo "%M'%S\""
  } elseif {[expr 10*$spix]>$platz} {
#    puts "1s 10s"
    set ti 1; set li 10; set fo "%M'%S\""
  } elseif {[expr 30*$spix]>$platz} {
#    puts "10s 30s"
    set ti 10; set li 30; set fo "%M'%S"
  } elseif {$mpix>$platz} {
#    puts "10s 1min"
    set ti 10; set li 60; set fo "%H:%M'"
  } elseif {[expr 5*$mpix]>$platz} {
#    puts "1min 5min"
    set ti 60; set li 300; set fo "%H:%M'"
  } elseif {[expr 10*$mpix]>$platz} {
#    puts "1min 10min"
    set ti 60; set li 600; set fo "%H:%M'"
  } elseif {[expr 30*$mpix]>$platz} {
#    puts "10min 30min"
    set ti 600; set li 1800; set fo "%H:%M'"
  } elseif {$hpix>$platz} {
 #   puts "10min 1h"
    set ti 600; set li 3600; set fo "%a;%Hh"
  } elseif {[expr 10*$hpix]>$platz} {
#    puts "1h 10h"
    set ti 3600; set li 36000; set fo "%a;%Hh"
  } else {
#    puts "1h 1d"
    set ti 3600; set li 86400; set fo "%a"
  }
#puts "ti=$ti"
  for {set i [expr $imin-$imin%$ti+$ti]} {$i<$imax} {incr i $ti} {
    set x [$hi_win transevx $i]
    $cv(win) create line\
      $x $yso\
      $x $ysu\
      -tags Xlabel\
      -width 0
  }
  for {set i [expr $imin-$imin%$li+$li]} {$i<$imax} {incr i $li} {
    set x [$hi_win transevx $i]
    $cv(win) create line\
      $x $yo\
      $x $yu\
      -tags Xlabel\
      -width 2
    if {($x>$leftx) && ($x<$rightx)} {
      $cv(win) create text $x $ypos\
          -anchor n\
          -font $cv(font)\
          -tags Xlabel\
          -text [clock format $i -format $fo]
    }
    set leftx $x
  }
}

proc make_y_scale {} {
  global cv hi hi_win
  set ymax [$hi_win ymax]
  set exp [expr round(log10($ymax))]
  set a [expr $ymax/pow(10.0,$exp)]
  if {$a<=1} {
    set inc 0.1
  } elseif {$a<=2} {
    set inc 0.2
  } elseif {$a<=5} {
    set inc 0.5
  } else {
    set inc 1
  }
  set inc [expr $inc*pow(10, $exp)]
  set ylist {}
  for {set y 0} {$y<$ymax} {set y [expr $y+$inc]} {
    lappend ylist [delnull $y]
  }
  lappend ylist [delnull $ymax]

  $cv(win) delete Ylabel
  set lasty 0
  foreach i $ylist {
    set y [$hi_win transevy $i]
    $cv(win) create line\
        [expr $hi(x0)-$cv(ticl)] $y\
        $hi(x0) $y\
        -tags Ylabel
    $cv(win) create line\
        $hi(x1) $y\
        [expr $hi(x1)+$cv(ticl)] $y\
        -tags Ylabel
    if {$lasty==0 || $y<$lasty} {
      $cv(win) create text [expr $hi(x1)+$cv(ticl)+$cv(space)] $y\
          -anchor w\
          -font $cv(font)\
          -tags Ylabel\
          -text $i
      set lasty [expr $y-$cv(th_)]
    }
  }
}

proc xh_cross {action x xv y yv} {
  global cv hi
  if {$action=="delete"} {
    $cv(win) delete Cross
  } else {
    if {$action=="move"} {$cv(win) delete Cross}
    $cv(win) create text $x [expr $hi(y0)-$cv(ticl)-$cv(space)]\
        -anchor s\
        -font $cv(font)\
        -tags Cross\
        -text [clock format [lindex [split $xv .] 0] -format "%a %H:%M:%S"]
    $cv(win) create text [expr $hi(x0)-$cv(ticl)-$cv(space)] $y\
        -anchor e\
        -font $cv(font)\
        -tags Cross\
        -text [lindex [split $yv .] 0]
#        -text [format "%.2f" $yv]
  }
}

proc cv_conf {} {
  global cv hi hi_win global_setup
  
  set cv(width) [winfo width $cv(win)]
  set cv(height) [winfo height $cv(win)]
  set global_setup(width) $cv(width)
  set global_setup(height) $cv(height)
  set hi(x0) $cv(leftdist)
  set hi(x1) [expr $cv(width)-$cv(rightdist)]
  set hi(y0) $cv(topdist)
  set hi(y1) [expr $cv(height)-$cv(bottomdist)]
  set hi(width) [expr $hi(x1)-$hi(x0)]
  set hi(height) [expr $hi(y1)-$hi(y0)]
  $hi_win configure -width [expr $hi(width)+1] -height [expr $hi(height)+1]
#  $icv(win) coords Rahmen 0 0 $icv(width) $icv(height)
  $hi_win transxoffs $cv(leftdist)
  $hi_win transyoffs $cv(topdist)
#  make_x_scale
  make_y_scale
}

proc xh_scan_start {} {
  global hi hi_win
  $hi_win xscalemode constant
  set hi(xscan) 1
  .bar configure -background #f00
}

proc xh_scan_stop {} {
  global hi hi_win
  $hi_win xscalemode lastdata 
  set hi(xscan) 0
  .bar configure -background #00f
}

proc xh_shift_pixel {dir mode} {
  global hi hi_win
  if {!$hi(xscan)} {xh_scan_start}
  if {$mode==1} {
    set d 1
  } elseif {$mode==2} {
    set d 10
  } else {
    set d [expr [$hi_win cget -width]/2]
  }
  if {$dir=="-"} {set d -$d}
  $hi_win xshift $d
}

proc xh_mouse_down {x} {
  global hi
  if {!$hi(xscan)} {xh_scan_start}
  set hi(scanx) $x
}

proc xh_mouse_move {x state} {
  global hi hi_win
  set f 1;
  if {[expr $state&1]} {set f 2}
  if {[expr $state&4]} {set f [expr 10*$f]}
  $hi_win xshift [expr ($hi(scanx)-$x)*$f]
  set hi(scanx) $x
}

proc xh_space {} {
  global hi hi_win
  $hi_win xmax [$hi_win lastx]
  if {$hi(xscan)} {xh_scan_stop}
}

proc xh_m {} {
  global global_setup
  set global_setup(show_menubar) [expr 1-$global_setup(show_menubar)]
  pack_all
}

proc xh_s {} {
  global global_setup
  set global_setup(show_scrollbar) [expr 1-$global_setup(show_scrollbar)]
  pack_all
}

proc xh_l {} {
  global global_setup
  set global_setup(show_legend) [expr 1-$global_setup(show_legend)]
  pack_all
}

proc cv_scroll {x} {
global hi_win
$hi_win xmax [expr [$hi_win firstx]+$x]
}

proc cv_init {} {
  global cv hi hi_win global_setup

# Konstanten
  set cv(ticl) 10
  set cv(sticl) 7
  set cv(space) 3

# Text vorbereiten und Groesse bestimmen
  set cv(font) $global_setup(cv_font)
  set text [$cv(win) create text 0 0 -anchor center -font $cv(font)\
      -text 88888]
  set cv(bb) [$cv(win) bbox $text]
  set cv(tw_) [expr [lindex $cv(bb) 2]-[lindex $cv(bb) 0]]
  set cv(th_) [expr [lindex $cv(bb) 3]-[lindex $cv(bb) 1]]
  $cv(win) delete $text
  set text [$cv(win) create text 0 0 -anchor center -font $cv(font)\
      -text 88:88:88]
  set cv(bb) [$cv(win) bbox $text]
  set cv(ttw) [expr [lindex $cv(bb) 2]-[lindex $cv(bb) 0]]
  $cv(win) delete $text

# Rand fuer Beschriftung festlegen
  set cv(rightdist) [expr $cv(ticl)+2*$cv(space)+$cv(tw_)]
  set cv(leftdist) $cv(rightdist)
  set cv(bottomdist) [expr $cv(ticl)+2*$cv(space)+$cv(th_)]
  set cv(topdist) $cv(bottomdist)

# history window erzeugen
  set hi(win) [
    histowin $cv(win).plot -bg gray90
  ]
  set hi_win $hi(win)
  set hi(xscan) 0
  bind $hi(win) <Key-Left> {xh_shift_pixel + 1}
  bind $hi(win) <Key-Right> {xh_shift_pixel - 1}
  bind $hi(win) <Shift-Key-Left> {xh_shift_pixel + 2}
  bind $hi(win) <Shift-Key-Right> {xh_shift_pixel - 2}
  bind $hi(win) <Control-Key-Left> {xh_shift_pixel + 3}
  bind $hi(win) <Control-Key-Right> {xh_shift_pixel - 3}
  bind $hi(win) <Button-1> xh_space
  bind $cv(win) <Button-1> xh_space
  bind $hi(win) <Key-space> xh_space
  bind $cv(win) <Key-space> xh_space
  bind $hi(win) <Button-2> {xh_mouse_down %x}
  bind $hi(win) <Button2-Motion> {xh_mouse_move %x %s}
  bind $hi(win) <Key-M> xh_m
  bind $cv(win) <Key-M> xh_m
  bind $hi(win) <Key-m> xh_m
  bind $cv(win) <Key-m> xh_m
  bind $hi(win) <Key-S> xh_s
  bind $cv(win) <Key-S> xh_s
  bind $hi(win) <Key-s> xh_s
  bind $cv(win) <Key-s> xh_s
  bind $hi(win) <Key-L> xh_l
  bind $cv(win) <Key-L> xh_l
  bind $hi(win) <Key-l> xh_l
  bind $cv(win) <Key-l> xh_l
  $cv(win) create window $cv(leftdist) $cv(topdist)\
    -anchor nw\
    -window $hi(win)
#  $icv(win) create rectangle 0 0 1 1 -tags Rahmen

# Binding fuer Configure-Events festlegen
  bind $cv(win) <Configure> cv_conf
#  update idletasks
  cv_conf
  $hi(win) xmax [clock seconds]
  $hi(win) xdiff $global_setup(x_axis_diff)
  $hi(win) ymin 0
  $hi(win) ymax $global_setup(y_axis_max)
  $hi(win) transxoffs $cv(leftdist)
  $hi(win) transyoffs $cv(topdist)
  $hi(win) xscalecommand make_x_scale
  $hi(win) crosscommand xh_cross
  make_y_scale
  bind $cv(scroll) 
  $cv(scroll) configure -command "xh_scan_start; $hi_win xview"
  $hi(win) scrollcommand "$cv(scroll) set"
}
