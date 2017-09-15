# $ZEL: wait_for_ready.tcl,v 1.6 2003/02/04 19:28:06 wuestner Exp $
# copyright:
# 1998 P. Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
#
# global_veds_running($ved)
# global_veds_writing($ved)
# global_veds_reading($ved)
#

proc ldiff {list1 list2} {
  set ll {}
  foreach i $list1 {
    if {[lsearch -exact $list2 $i]==-1} {lappend ll $i}
  }
  return $ll
}

proc lmatch {list1 list2} {
  set ll {}
  foreach i $list2 {
    if {[lsearch -exact $list1 $i]>=0} {lappend ll $i}
  }
  return $ll
}

proc vedtextlist {list} {
  set num [llength $list]
  if {$num==0} return ""
  set tt [[lindex $list 0] name]
  if {$num>1} {
    incr num -1
    for {set i 1} {$i<$num} {incr i} {
      append tt ", [[lindex $list $i] name]"
    }
    append tt " and [[lindex $list $num] name]"
  }
  return $tt
}

proc wait_for_stopped {veds} {
  global global_veds_running

  output "wait for [vedtextlist $veds] to become \"stopped\"" tag_blue

  set cc $veds
  while {[llength $cc]>0} {
    set cc0 {}
    foreach ved $veds {
      if {$global_veds_running($ved)==0} {lappend cc0 $ved}
    }
    if {[llength $cc0]>0} {
      set cc1 [lmatch $cc $cc0]
      set cc  [ldiff $cc $cc1]
      output_append "  ...[vedtextlist $cc1] OK" tag_blue
    }
    if {[llength $cc]>0} {vwait global_veds_running}
  }
  return 0
}

proc wait_for_do_ready {veds quiet} {
  global global_veds_writing
  if {!$quiet} {
    output "wait for dataouts of [vedtextlist $veds] to become ready" tag_blue
  }
  set l0 {}
  foreach ved $veds {
    if {$global_veds_writing($ved)==0} {lappend l0 $ved}
  }
  set l1 [lmatch $l0 $veds]
  if {[llength $l1]>0} {
    output_append " ... [vedtextlist $l1] OK" tag_blue
    return $l1
  }
  vwait global_veds_writing
  set l0 {}
  foreach ved $veds {
    if {$global_veds_writing($ved)==0} {lappend l0 $ved}
  }
  set l1 [lmatch $l0 $veds]
  output_append "  ...[vedtextlist $l1] OK" tag_blue
  return $l1
}

proc wait_for_di_ready {veds quiet} {
  global global_veds_reading
  if {!$quiet} {
    output "wait for datains of [vedtextlist $veds] to become ready" tag_blue
  }
  set l0 {}
  foreach ved $veds {
    if {$global_veds_reading($ved)==0} {lappend l0 $ved}
  }
  set l1 [lmatch $l0 $veds]
  if {[llength $l1]>0} {
    output_append " ... [vedtextlist $l1] OK" tag_blue
    return $l1
  }
  vwait global_veds_reading
  set l0 {}
  foreach ved $veds {
    if {$global_veds_reading($ved)==0} {lappend l0 $ved}
  }
  set l1 [lmatch $l0 $veds]
  output_append "  ...[vedtextlist $l1] OK" tag_blue
  return $l1
}
