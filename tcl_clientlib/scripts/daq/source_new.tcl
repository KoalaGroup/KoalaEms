# $ZEL: source_new.tcl,v 1.2 2003/02/04 19:27:58 wuestner Exp $
# copyright:
# 2000 Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc source_new {file} {
  global global_sourcelist

  if {![info exists $global_sourcelist]} {
    array set $global_sourcelist {}
  }
  set idx [array size $global_sourcelist]
  set file [file join [pwd] $file]

  if [catch {set mtime [file mtime $file]} mist] {
    output "stat setupinclude $file:\
      $mist" tag_red
    set mtime 0
  }
  
  set ${global_sourcelist}($idx) "$file $mtime"

  uplevel ::source_orig $file
}

proc dummy_source_new {} {
}
