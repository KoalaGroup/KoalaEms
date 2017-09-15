# $ZEL: unsol_runtime_Mismatch.tcl,v 1.2 2002/10/19 23:07:01 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc decode_unsol_runtime_Mismatch {space v h d} {
  set errno 0
  set signal 0

  output_append "  Mismatch" tag_blue

  if {$errno} {output_append "  BSD-Error: [strerror_bsd $errno]" tag_blue}
  if {$signal} {output_append "  BSD-Signal: [strsignal_bsd $signal]" tag_blue}
  return 1
}
