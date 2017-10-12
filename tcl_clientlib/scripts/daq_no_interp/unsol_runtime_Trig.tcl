# $Id: unsol_runtime_Trig.tcl,v 1.2 2000/08/31 19:43:27 wuestner Exp $
# copyright 2000
#   Peter  Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc decode_unsol_runtime_Trig {space v h d} {
  #output_append "  Trig" tag_blue

  set eventcnt [lindex $d 1]
  output_append "  Error in trigger procedure; eventcnt=$eventcnt" tag_red

  #set code [lindex $d 0] == rtErr_ExecProcList
  set subcode [lindex $d 2]
  switch $subcode {
    1 {
      set errno [lindex $d 3]
      output_append "    error reading trigger module: [strerror $errno]" tag_red
    }
    2 {
      output_append "    SYNC_MBX_EOC not set in mailbox" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      output_append "    [format {mbx=0x%08x state=0x%08x} $mbx $state]" tag_red
    }
    3 {
      output_append "    SYNC_GET_EOC not set in status" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      output_append "    [format {mbx=0x%08x state=0x%08x} $mbx $state]" tag_red
    }
    4 {
      set errno [lindex $d 3]
      output_append "    error during select: [strerror $errno]" tag_red
    }
    5 {
      set errno [lindex $d 3]
      output_append "    short read: [strerror $errno]" tag_red
    }
    6 {
      output_append "    eventcounter mismatch" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      set evc [lindex $d 5]
      output_append "    [format {mbx=0x%08x state=0x%08x evc=%d} $mbx $state $evc]" tag_red
    }
    7 {
      output_append "    subevent invalid" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      output_append "    [format {mbx=0x%08x state=0x%08x} $mbx $state]" tag_red
    }
    8 {
      output_append "    trigger id =0" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      output_append "    [format {mbx=0x%08x state=0x%08x} $mbx $state]" tag_red
    }
    9 {
      output_append "    no EOC or no INH" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      output_append "    [format {mbx=0x%08x state=0x%08x} $mbx $state]" tag_red
    }
    default {
      output_append "    unknown subcode $subcode" tag_red
    }
  }
  return 1
}
