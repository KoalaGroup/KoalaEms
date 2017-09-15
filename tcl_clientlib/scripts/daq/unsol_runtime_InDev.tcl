# $ZEL: unsol_runtime_InDev.tcl,v 1.2 2003/02/04 19:28:03 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc decode_unsol_runtime_InDev {space v h d} {
  set di_idx [lindex $d 1]
  set typ [lindex $d 2]
  output_append "  Input Device idx=$di_idx" tag_blue
  switch $typ {
    2 {
      output_append "  wrong selection: [lindex $d 3]" tag_blue
    }
    3 {
      set errno [lindex $d 3]
      output_append "  read head, errno=$errno ([strerror $space $errno])" tag_blue
    }
    4 {
      output_append "  wrong endien, endien=[format {0x%x} [lindex $d 3]]" tag_blue
    }
    5 {
      set errno [lindex $d 3]
      output_append "  read data, errno=$errno ([strerror $space $errno])" tag_blue
    }
    6 {
      output_append "  old VED-info" tag_blue
    }
    7 {
      output_append "  unknown version of VED-info ([lindex $d 3])" tag_blue
    }
    8 {
      output_append "  exception in connect" tag_blue
    }
    9 {
      set errno [lindex $d 3]
      output_append "  connect (getsockopt(SO_ERROR)), errno=$errno ([strerror $space $errno])" tag_blue
    }
    10 {
      set errno [lindex $d 3]
      output_append "  connect errno=$errno ([strerror $space $errno])" tag_blue
    }
    11 {
      output_append "  connect, no task" tag_blue
    }
    12 {
      output_append "  exception in accept" tag_blue
    }
    13 {
      set errno [lindex $d 3]
      output_append "  accept, errno=$errno ([strerror $space $errno])" tag_blue
    }
    14 {
      set errno [lindex $d 3]
      output_append "  fcntl(O_NDELAY), errno=$errno ([strerror $space $errno])" tag_blue
    }
    15 {
      output_append "  accept, no task" tag_blue
    }
    default {output_append "  unknown type $typ" tag_blue}
  }
  return 1
}
