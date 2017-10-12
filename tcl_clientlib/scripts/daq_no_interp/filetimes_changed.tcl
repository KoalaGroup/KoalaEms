# $Id: filetimes_changed.tcl,v 1.1 2000/08/06 19:41:10 wuestner Exp $
# copyright:
# 2000 Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc filetimes_changed {arr} {
  global global_verbose
  if {![info exists $arr]} {
    if {$global_verbose} {output "array $arr does not exist"}
    return 1
  }
  foreach i [lsort -integer [array names $arr]] {
    set list [set ${arr}($i)]
    set file [lindex $list 0]
    set mtime  [lindex $list 1]
    if {$global_verbose} {output_append_nonl "checking $file: "}
    if [catch {set nmtime [file mtime $file]} mist] {
      if {$global_verbose} {output_append ""}
      if {$global_verbose} {output_append "stat setupinclude $file:\
        $mist" tag_red}
      return 1
    }
    if {$mtime!=$nmtime} {
      if {$global_verbose} {output_append "changed at [clock format $nmtime]"}
      return 1
    } else {
      if {$global_verbose} {output_append "unchanged"}
    }
  }
  return 0
}
