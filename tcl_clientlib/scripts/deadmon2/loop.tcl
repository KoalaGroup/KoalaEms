# $ZEL: loop.tcl,v 1.1 2000/07/15 21:37:01 wuestner Exp $
# copyright:
# 1999 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc dummy_LOOP {} {}

namespace eval ::LOOP {

# list of the namespaces of all available VEDs
global global_setup
if {![info exists reload]} {
  variable spaces {}
  variable interval $global_setup(loop_delay)
  variable afterproc 0
  variable reload 1
}

proc add {space} {
  variable spaces
  lappend spaces $space
  #LOG::put "LOOP::add: list: $spaces"
  start
}

proc delete {space} {
  variable spaces
  set idx [lsearch -exact $spaces $space]
  if {$idx<0} {
    LOG::put "LOOP::delete: $space not found"
  } else {
    set spaces [lreplace $spaces $idx $idx]
    #LOG::put "LOOP::delete: list: $spaces"
    if {[llength $spaces]==0} stop
  }
}

proc run {} {
  variable interval
  variable spaces
  variable afterproc

  #LOG::put "after" tag_green
  foreach i $spaces {
    send_request $i
  }

  set afterproc [after $interval ::LOOP::run]
}

proc start {} {
  stop
  run
}

proc stop {} {
  variable afterproc
  after cancel $afterproc
}

proc clear {} {
  variable spaces

  #LOG::put "clear" tag_green
  foreach i $spaces {
    reset_statistics $i
  }
}

}
