# global vars in this file:
#
# global_farben
# global_farbidx
#

set global_farben {#000 #00f #0ff #f00 #f0f #ff0 #fff #008 #080 #088 #08f #0f8 \
#800 #808 #80f #880 #888 #88f #8f0 #8f8 #8ff #f08 #f80 #f88 #f8f #ff8}
set global_farben {#000 #f00 #0f0 #00f #0ff #f0f #ff0 #088 #808 #880 #800 #080 \
#008 #f80 #f08 #8f0 #0f8 #80f #08f #f88 #8f8 #88f #8ff #f8f #ff8}
set global_farbidx 0

proc newcolor {} {
  global global_farben global_farbidx
  if {$global_farbidx>=[llength $global_farben]} {set global_farbidx 0}
  set f [lindex $global_farben $global_farbidx]
  incr global_farbidx
  return $f
}
