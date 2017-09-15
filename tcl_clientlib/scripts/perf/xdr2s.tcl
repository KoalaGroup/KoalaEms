# $Id: xdr2s.tcl,v 1.2 1998/06/29 18:02:42 wuestner Exp $
#

proc xdr2s {num list} {
  set c 0
  for {set w 0} {$c<$num} {incr w} {
    set word [lindex $list $w]
    for {set b 24} {($b>=0) && ($c<$num)} {incr b -8} {
      set byte [expr ($word>>$b)&0xff]
      append res [format {%c} $byte]
      incr c
    }
  }
  return $res
}

proc xdrl2s {list idx} {
  set num [lindex $list $idx]
  set words [expr ($num+3)/4]
  set rest [lrange $list [expr $idx+1] end]
  set res [lreplace $list $idx [expr $idx+$words] [xdr2s $num $rest]]
  return $res
}
