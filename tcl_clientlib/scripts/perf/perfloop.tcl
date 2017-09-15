# $Id: perfloop.tcl,v 1.3 1999/04/09 12:32:50 wuestner Exp $
#

proc got_error {ved args} {
  puts "got no data from VED $ved"
  puts "args=\{$args\}"
}

namespace eval ::Loop {}

proc ::Loop::add {ved} {
  variable veds
  set namespace ::ns_${ved}
  lappend veds $ved
  set ${namespace}::ro(time) 0
}

proc ::Loop::delete {ved} {
  variable veds
  set idx [lsearch -exact $veds $ved]
  set veds [lreplace $veds $idx $idx]
}

proc ::Loop::ask {ved} {
  set namespace ::ns_${ved}
  if {[set ${namespace}::ready]==1} {
    $ved readout status 1 7 : got_ro_status $ved ? got_error  $ved
    foreach i [set ${namespace}::dataoutdom] {
      $ved dataout status $i 1 : got_do_status $ved $i ? got_error  $ved $i
    }
  }
}

proc ::Loop::run {} {
  variable interval
  variable veds
  foreach ved $veds {
    ask $ved
  }
  after [expr $interval*1000] ::Loop::run
}

proc ::Loop::start {inter} {
  variable interval $inter
  variable veds
  set veds {}
  after 1000 ::Loop::run
}
