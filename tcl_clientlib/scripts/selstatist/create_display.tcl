# $Id: create_display.tcl,v 1.2 1998/10/07 10:37:17 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#
# global vars:
#
# global_setup(veds)
# global_status($ved)
# global_count($ved)
# 

#-----------------------------------------------------------------------------#
proc dummy_display {} {
}

namespace eval ::Display {}

proc ::Display::create_display {frame} {
  global global_setup
  variable outer_frame

  set outer_frame $frame
  foreach ved $global_setup(veds) {
    set f [frame $frame.$ved -relief ridge -borderwidth 4]
    #set ::${ved}::ved $ved
    set ::${ved}::frame $f
    set ::${ved}::rows(di) 0
    set ::${ved}::rows(do) 0
    label $f.name -text $ved -anchor w
    entry $f.dt -relief flat -textvariable ::${ved}::dt -width 6 -justify right
    frame $f.di -relief raised -borderwidth 2
    frame $f.do -relief raised -borderwidth 2
    grid configure $f.name $f.dt -sticky ew
    grid configure $f.di $f.do -sticky ewn
    pack $f -side top -fill both -expand 1
  }
}

proc ::Display::update_display {dio ved list} {
  set ::${ved}::list $list
  set ::${ved}::dio $dio
  namespace eval ::${ved} {
    global $ved
    set num [llength $list]
    if {$num!=$rows($dio)} {
      if {$num<$rows($dio)} {
        for {set i $num} {$i<$rows($dio)} {incr i} {
          destroy $frame.$dio.${i}l
          destroy $frame.$dio.${i}v
          destroy $frame.$dio.${i}b
        }
      } else {
        for {set i $rows($dio)} {$i<$num} {incr i} {
          entry $frame.$dio.${i}l \
            -textvariable text_${ved}_${dio}($i) -justify left -width 0 \
            -relief flat
          entry $frame.$dio.${i}v -relief sunken \
            -textvariable val_${ved}_${dio}($i) -width 6 -justify right
          canvas $frame.$dio.${i}b -height 3 -background #d9d9d9 -width 100
          set tag_${dio}($i) [$frame.$dio.${i}b create rectangle 0 0 0 3 -fill blue]
          grid configure $frame.$dio.${i}l $frame.$dio.${i}v -sticky ew
          grid configure $frame.$dio.${i}b -sticky ew -columnspan 2
        }
      }
      set rows($dio) $num
    }
    set i 0
    foreach n $list {
      set text_${ved}_${dio}($i) $n
      set v [set ${ved}($n)]
      set val_${ved}_${dio}($i) [format {%.2f} $v]
      set w [winfo width $frame.$dio.${i}b]
      set p [expr int($v*$w/100.)-1]
      $frame.$dio.${i}b delete [set tag_${dio}($i)]
      set tag_${dio}($i) [$frame.$dio.${i}b create rectangle 0 0 $p 3 -fill blue]
      incr i
    }
  }
}

#-----------------------------------------------------------------------------#
#-----------------------------------------------------------------------------#
