# $ZEL: sandboxprocs.tcl,v 1.5 2009/04/01 16:29:58 wuestner Exp $
# copyright 1998
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
# these Macintosh versions of source are not supported here:
#  source -rsrc resourceName ?fileName?
#  source -rsrcid resourceId ?fileName?

proc sandbox_source {sandbox name} {
  #output "sandbox_source $sandbox $name" tag_orange
  #output "open files: [file channels]" tag_blue

  set name [file normalize [file join [pwd] $name]]
  if [catch {set mtime [file mtime $name]} mist] {
    set mtime 0
  }
  $sandbox eval set ::sourcelist($name) $mtime
  $sandbox invokehidden source $name
}

proc sandbox_open {sandbox name} {
  #output "sandbox_open $sandbox $name" tag_blue
  #output "open files: [file channels]" tag_blue

  set name [file normalize [file join [pwd] $name]]
  if [catch {set mtime [file mtime $name]} mist] {
    set mtime 0
  }
  $sandbox eval set ::sourcelist($name) $mtime
  $sandbox invokehidden open $name RDONLY
}

proc sandbox_creat {sandbox name args} {
  #output "sandbox_creat $sandbox $name $args" tag_blue
  #output "open files: [file channels]" tag_blue

  if {![string equal [file pathtype $name] "absolute"]} {
    error "$name is not absolute"
  }
  set name [file normalize $name]
  $sandbox eval set ::sourcelist($name) 0
  $sandbox invokehidden open $name $args
}

proc sandbox_output {ident text args} {
  output "$ident: $text" [split $args]
}

proc sandbox_output_append {ident text args} {
  output_append "$ident: $text" [split $args]
}

proc sandbox_ved_is_create {ved space idx id} {
  set is [$ved is create $idx $id]
  regsub {_.*_} $is {_} name
  $space alias $name $is
  return $name
}

proc sandbox_get_vedcommand {myspace vedname} {
    global global_setupdata
    foreach ved [ems_openveds] {
        if {[string equal [$ved name] $vedname]} {
            $myspace alias $ved $ved
            return $ved
        }
    }
}
