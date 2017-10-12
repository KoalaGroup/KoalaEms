# $ZEL: log.tcl,v 1.1 2000/07/15 21:37:01 wuestner Exp $
# copyright:
# 12004 P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#

namespace eval ::LOG {

proc create {w} {
}

proc verbose {name1 name2 op} {
  global global_verbose
  if {$global_verbose} {
    output "logmodus switched to verbose" tag_orange
  } else {
    output "logmodus switched to nonverbose" tag_orange
  }
}

proc warning {} {
}

proc put_raw {line args} {
    puts $line
}

proc append {line args} {
    puts $line
}

proc put {line args} {
    puts $line
}

proc append_nonl {line args} {
    puts -nonewline $line
}

proc put_list {lines args} {
  foreach i $lines {
    put $i
  }
}

proc log_see_end {} {
}

}
