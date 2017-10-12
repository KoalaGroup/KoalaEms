proc create_header_from_file {filename header} {
  upvar $header head

  if [catch {set file [open $filename RDONLY]} mist] {
    output "open file $filename: $mist" tag_red
    return -1
  }
  set head {}
  lappend head "Key: userfile:$filename"
  set res [gets $file line]
  while {$res>=0} {
    lappend head $line
    set res [gets $file line]
  }
  close $file
  return 0
}
