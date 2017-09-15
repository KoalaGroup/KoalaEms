# $Id: xdr_opaque.tcl,v 1.1 1998/09/30 13:38:29 wuestner Exp $
# © 1998 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

proc xdr_opaque_extract {o_list} {
  upvar $o_list l
  set num [lindex $l 0]
  set l [lrange $l 1 end]
  set i_list {}

  for {set c 0} {$c<$num} {} {
    set word [lindex $l 0]
    set l [lrange $l 1 end]
    for {set b 24} {($b>=0) && ($c<$num)} {incr b -8} {
      set byte [expr ($word>>$b)&0xff]
      lappend i_list $byte
      incr c
    }
  }
  return $i_list
}

proc xdr_opaque_create {i_list} {
  set num [llength $i_list ]
  lappend o_list $num
  set inum [expr ($num+3)/4]
  for {set i 0; set j 0} {$i<$inum} {incr i} {
    set word 0
    for {set b 24} {($b>=0) && ($j<$num)} {incr b -8; incr j} {
      set c [lindex $i_list $j]
      incr word [expr $c<<$b]
    }
    lappend o_list $word
  }
  return $o_list
}
