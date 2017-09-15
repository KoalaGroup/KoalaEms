proc max {a args} {
  set max $a
  foreach i $args {if {$max<$i} {set max $i}}
  return $max
}

#proc boxsize {width height font names} {
#  upvar $width w
#  upvar $height h
#  set w 0
#  set h 0
#  foreach i $names {
#    set extents [tkf_textextents $font $i]
#    set ww [expr "[lindex $extents 0]+[lindex $extents 1]"]
#    set hh [expr "[lindex $extents 3]+[lindex $extents 4]"]
#    set w [max $w $ww]
#    set h [max $h $hh]
#  }
#}

proc boxsize {width height win font names} {
  upvar $width w
  upvar $height h
  set w 0
  set h 0
  foreach i $names {
    lappend t [$win create text 0 0 -anchor center -font $font -text $i]
  }
  set box [eval $win bbox $t]
  eval "$win delete $t"
  set w [expr [lindex $box 2]-[lindex $box 0]]
  set h [expr [lindex $box 3]-[lindex $box 1]]
}

proc textbox {win box text font tags} {
  upvar $box b
  set color [$win cget -background]
  set b(eck) [$win create rectangle $b(x) $b(y)\
      [expr $b(x)+$b(w)] [expr $b(y)+$b(h)] -fill $color\
      -tags [concat $tags BOX BOTH]]
  set b(text) [$win create text [expr $b(x)+$b(w)/2] [expr $b(y)+$b(h)/2]\
      -text $text -font $font -tags [concat $tags TEXT BOTH]]
}

proc connectboxes {win box1 box2} {
  upvar $box1 b1
  upvar $box2 b2
  $win create line [expr $b1(x)+$b1(w)/2] [expr $b1(y)+$b1(h)]\
      [expr $b2(x)+$b2(w)/2] $b2(y) -arrow last
}
