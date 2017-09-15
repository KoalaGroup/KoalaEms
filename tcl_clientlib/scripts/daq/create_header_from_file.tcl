# $ZEL: create_header_from_file.tcl,v 1.5 2009/03/29 20:09:35 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
proc create_header_from_file {filename header} {
  upvar $header head
  global clockformat

  if [catch {file lstat $filename stat} mist] {
    output "lstat $filename: $mist" tag_blue
    set stat(mtime) 0
    set stat(size) -1
  }

  if [catch {set file [open $filename RDONLY]} mist] {
    output "open file $filename: $mist" tag_red
    return -1
  }
  set head {}
  lappend head "Key: userfile:$filename"
  lappend head "mtime: [clock format $stat(mtime) -format $clockformat]"
  lappend head "size: $stat(size)"
  lappend head {}
  if {$stat(size)>60000} {
     set tag tag_red
  } else {
     set tag tag_green
  }
  output  "header_from_file: $filename $stat(size) ([clock format $stat(mtime) -format $clockformat])" $tag
  if {$stat(size)<=60000} {
    set res [gets $file line]
    while {$res>=0} {
      lappend head $line
      set res [gets $file line]
    }
    close $file
  }
  return 0
}
