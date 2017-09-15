#$Id: colorname.tcl,v 1.2 1998/05/14 21:22:56 wuestner Exp $
#
# static_color_rgb_init
# 
#
#-----------------------------------------------------------------------------#

proc readrgbtab {} {
  global static_color_rgb_init static_color_rgb

  set rgbtab [exec showrgb]
  set rgbtab [split $rgbtab "\n"]
  #set rgbtab [lrange $rgbtab 0 10]
  foreach i $rgbtab {
    set i [split $i " \t"]
    set p [lsearch -exact $i {}]
    while {$p>=0} {
      set i [lreplace $i $p $p]
      set p [lsearch -exact $i {}]
    }
    set r [lindex $i 0]
    set g [lindex $i 1]
    set b [lindex $i 2]
    set name [lindex $i 3]
    for {set j 4} {$j<[llength $i]} {incr j} {
      append name " [lindex $i $j]"
    }
    set static_color_rgb($name) "$r $g $b"
    #puts "$name: $static_color_rgb($name)"
  }
  set static_color_rgb_init 1
}

#-----------------------------------------------------------------------------#

proc findrgbtab {color} {
  global static_color_rgb_init static_color_rgb

  if {![info exists static_color_rgb_init]} readrgbtab

  if [info exists static_color_rgb($color)] {
    set rgb $static_color_rgb($color)
  } else {
    set rgb {0 0 0}
  }
  return $rgb
}

#-----------------------------------------------------------------------------#

# Input: beliebige Farbe (#xxx, #xxxxxx, #xxxxxxxxx, name)
# Output: Liste mit 3 Elementen {R G B} im Intervall (0.0 1.0)
proc color2rgb {color} {
  global static_color_rgb_init

  if {[string index $color 0]=={#}} {
    set color [string range $color 1 end]
    set l [string length $color]
    set n [expr $l/3]
    set rest [expr $l%3]
    if {$rest!=0} {error "Wrong format for color"}
    set r 0x[string range $color 0 [expr $n-1]]
    set g 0x[string range $color $n [expr 2*$n-1]]
    set b 0x[string range $color [expr 2*$n] end]
    set div 0x
    for {set i 0} {$i<$n} {incr i} {append div f}
    set R [expr double($r)/$div]
    set G [expr double($g)/$div]
    set B [expr double($b)/$div]
  } else {
    set rgb [findrgbtab $color]
    set r [lindex $rgb 0]
    set g [lindex $rgb 1]
    set b [lindex $rgb 2]
    set R [expr double($r)/255.0]
    set G [expr double($g)/255.0]
    set B [expr double($b)/255.0]
  }
  return "$R $G $B"
}

#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
