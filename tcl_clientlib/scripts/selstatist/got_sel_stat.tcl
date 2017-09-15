proc extractstring {list} {
  upvar $list l
  set num [lindex $l 0]
  set l [lrange $l 1 end]

  for {set c 0} {$c<$num} {} {
    set word [lindex $l 0]
    set l [lrange $l 1 end]
    for {set b 24} {($b>=0) && ($c<$num)} {incr b -8} {
      set byte [expr ($word>>$b)&0xff]
      append res [format {%c} $byte]
      incr c
    }
  }
  return $res
}

proc extractfloat {list} {
  upvar $list l
  set scale [lindex $l 0]
  set val [lindex $l 1]
  set l [lrange $l 2 end]
  for {} {$scale>0} {incr scale -1} {set val [expr $val/10.]}
  return $val
}

proc got_sel_stat {ved args} {
  set name [$ved name]
  global $name
  set num [lindex $args 0]
  set l [lrange $args 1 end]
  if [info exists $name] {unset $name}
  for {set i 0} {$i<$num} {incr i} {
    set sname [extractstring l]
    regsub {_cluster_read|_cluster_write} $sname {} sname
    regsub {_handle_tape} $sname { (T)} sname
    set v1 [extractfloat l]
    set v2 [extractfloat l]
    if {$sname!="select"} {
      set pro [expr $v2/($v1+$v2)*100]
      set ${name}($sname) $pro
    }
  }
  set tmp {}
  foreach i [array names $name] {
    if {[regexp {^di_} $i] && ![regexp {accept} $i]} {lappend tmp $i}
  }
  set tmp [lsort $tmp]
  foreach i [array names $name] {
    if {[regexp {^trigger} $i]} {lappend tmp $i}
  }
  ::Display::update_display di $name $tmp
  set tmp {}
  foreach i [array names $name] {
    if {[regexp {^do_} $i]} {lappend tmp $i}
  }
  set tmp [lsort $tmp]
  ::Display::update_display do $name $tmp
}

proc got_sel_stat_err {ved conf} {
  #puts [$conf print]
  $conf delete
}
