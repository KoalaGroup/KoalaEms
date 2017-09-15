#$Id: tearoff.tcl,v 1.2 1998/05/14 21:23:02 wuestner Exp $

proc offtorn {var win newwin} {
  upvar $var lvar
  lappend lvar(tornoffs) $newwin 
}

proc checktornoffs {var} {
  upvar $var lvar

  set l {}
  foreach i $lvar(tornoffs) {
    if [winfo exists $i] {
      lappend l $i
    }
  }
  set lvar(tornoffs) $l
}
