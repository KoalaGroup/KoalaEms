# $Id: calculate_intsize.tcl,v 1.2 2000/08/06 19:41:10 wuestner Exp $
# copyright: 1998, 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc calculate_intsize {} {
for {set bits 1} {$bits<=64} {incr bits} {
  set val 1
  for {set i 1} {$i<$bits} {incr i} {
    set val [expr $val*2]
  }
  if {$val<0} {
    #puts $bits
    return $bits
  }
}
return 0
}
