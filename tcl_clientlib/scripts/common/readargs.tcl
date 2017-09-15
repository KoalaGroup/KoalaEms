#$Id: readargs.tcl,v 1.2 1998/05/14 21:22:59 wuestner Exp $
#
# extract args
#
# namedarg(<name>)=...
# posarg(index)=...
#
proc process_args {} {
  global namedarg posarg
  global argc argv
  set pos 0
  for {set i 0} {$i<$argc} {} {
    set xx [lindex $argv $i]
    if {[string index $xx 0]!="-"} {
      set posarg($pos) $xx
      incr pos
      incr i
    } else {
      set xx [string trimleft $xx -]
      incr i
      set yy [lindex $argv $i]
      if {[string index $yy 0]!="-"} {
        set namedarg($xx) $yy
        incr i
      } else {
        set namedarg($xx) ""
      }
    }
  }
}

proc printargs {} {
  global namedarg posarg
  foreach i [array names argv] {
    puts "-$i $argv($i)"
  }
  if {[array exists namedarg]} {
    foreach i [array names namedarg] {
      puts "-$i $namedarg($i)"
    }
  }
  if {[array exists posarg]} {
    foreach i [array names posarg] {
      puts "position $i $posarg($i)"
    }
  }
}
