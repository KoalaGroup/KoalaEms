# $ZEL: unsol_runtime_OutDev.tcl,v 1.3 2003/02/04 19:28:04 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc decode_unsol_runtime_OutDev {space v h d} {
  set do_idx [lindex $d 1]
  switch [lindex $d 2] {
    0 {
      set errno [lindex $d 3]
      output_append "  error in dataout $do_idx: errno=$errno ([strerror $space $errno])" tag_red
    }
    1 {
      output_append "$d"
      output_append "  error in dataout $do_idx: scsi-error:" tag_red
      output_append "    code=[format {0x%0x} [lindex $d 3]]" tag_red
      output_append "    key =[format {0x%0x} [lindex $d 4]]" tag_red
      output_append "    asc =[format {0x%0x} [lindex $d 5]]" tag_red
      output_append "    ascq=[format {0x%0x} [lindex $d 6]]" tag_red
      output_append "    eom =[format {0x%0x} [lindex $d 7]]" tag_red
      decode_scsi_error [lindex $d 3] [lindex $d 4] [lindex $d 5]\
        [lindex $d 6] [lindex $d 7]
    }
    2 {
      output_append "  error in dataout $do_idx: child terminated:" tag_red
      switch [lindex $d 3] {
        0 {output_append "    exitcode [lindex $d 4]" tag_red}
        1 {output_append "    signal [lindex $d 4]" tag_red}
      }
    }
    3 {
      output_append "  dataout $do_idx: end of media reached." tag_red
      stop_because_of_end_of_media $v $do_idx
    }
    4 {
      output_append "  dataout $do_idx: cannot initialise SCSI Device" tag_red
    }
  }
  return 1
}
