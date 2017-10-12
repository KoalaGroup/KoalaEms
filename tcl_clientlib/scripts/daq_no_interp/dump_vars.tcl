# $Id: dump_vars.tcl,v 1.1 1999/05/07 15:19:29 wuestner Exp $
# copyright:
# 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
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
