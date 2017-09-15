# $ZEL: decode_scsi.tcl,v 1.3 2003/02/04 19:27:51 wuestner Exp $
# copyright:
# 1999 P. Wuestner; Zentrallabor für Elektronik; Forschungszentrum Juelich
proc decode_scsi_error {code key asc ascq eom} {
  switch -- $code {
    112 {}
    113 {output_append "  deferred error" tag_red}
    default {output_append "  unknown errorcode [format {0x%0x} $code]" tag_red}
  }
  switch -- $key {
    0 {}
    1 {output_append "  Recovered Error" tag_red}
    2 {output_append "  Not Ready" tag_red}
    3 {output_append "  Medium Error" tag_red}
    4 {output_append "  Hardware Error" tag_red}
    5 {output_append "  Illegal Request" tag_red}
    6 {output_append "  Unit Attention" tag_red}
    7 {output_append "  Data Protected" tag_red}
    8 {output_append "  Blank Check" tag_red}
   11 {output_append "  Command Aborted" tag_red}
   13 {output_append "  Volume Overflow" tag_red}
   15 {output_append "  Miscompare" tag_red}
  }
  switch -- $asc/$ascq {
    "0/2" {output_append "  end of medium detected" tag_red}
    "3/2" {output_append "  excessive write errors" tag_red}
    "12/0" {output_append "  write error" tag_red}
    "15/1" {output_append "  random positioning error" tag_red}
    "15/2" {output_append "  mechanical positioning error" tag_red}
    "15/0" {output_append "  error" tag_red}
    "15/0" {output_append "  error" tag_red}
    "15/0" {output_append "  error" tag_red}
    "15/0" {output_append "  error" tag_red}
    "27/0" {output_append "  write protected" tag_red}
    "27/1" {output_append "  hardware write protected" tag_red}
    "27/2" {output_append "  logical unit software write protected" tag_red}
  }
}
