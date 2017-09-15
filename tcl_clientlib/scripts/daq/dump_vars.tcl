# $ZEL: dump_vars.tcl,v 1.2 2003/02/04 19:27:51 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
proc dump_vars {} {
  set vars [namespace eval :: {info vars}]
  foreach i $vars {
    if {$i=="i"} {
      puts "cannot use \"i\""
    } else {
      global $i
      if {[array exists $i]} {
        puts "array $i:"
        foreach j [array names $i] {
          puts "  $j: [set ${i}($j)]"
        }
      } else {
        puts "scalar $i: [set $i]"
      }
      puts ""
    }
  }
}

proc dump_array {arr} {
  global $arr
  output "$arr:"
  foreach i [array names $arr] {
    output "  $i: [set ${arr}($i)]"
  }
}
