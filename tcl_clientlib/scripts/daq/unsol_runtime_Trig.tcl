# $ZEL: unsol_runtime_Trig.tcl,v 1.6 2009/02/09 21:31:53 wuestner Exp $
# copyright 2000
#   Peter  Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#
proc decode_syncstatus {state} {
  set st {}
  if {$state&0x1} {append st { MASTER}}
  if {$state&0x2} {append st { GO}}
  if {$state&0x4} {append st { T4LATCH}}
  if {$state&0x8} {append st { TAW_ENA}}
  if {$state&0x70} {append st " OUT=[expr ($state&0x70)>>4]"}
  if {$state&0x80} {append st { AUX0_RST}}
  if {$state&0x3f00} {append st " INT_ENA=[format {%x} [expr ($state&0x3f00)>>8]]"}
  if {$state&0x8000} {append st { GSI}}
  if {$state&0xf0000} {append st " TRIGG=[format {%x} [expr ($state&0xf0000)>>16]]"}
  if {$state&0x100000} {append st { GO_RING}}
  if {$state&0x200000} {append st { VETO}}
  if {$state&0x400000} {append st { SI}}
  if {$state&0x800000} {append st { INH}}
  if {$state&0x7000000} {append st " IN=[expr ($state&0x7000000)>>24]"}
  if {$state&0x8000000} {append st { EOC}}
  if {$state&0x10000000} {append st { SI_RING}}
  if {$state&0x80000000} {append st { TDT_RING}}
  return $st
}

proc decode_unsol_runtime_Trig {space v h d} {
  #output_append "  Trig" tag_blue

  set eventcnt [lindex $d 1]
  output_append "  Error in trigger procedure; eventcnt=$eventcnt" tag_red

  #set code [lindex $d 0] == rtErr_ExecProcList
  set subcode [lindex $d 2]
  switch $subcode {
    1 {
      set errno [lindex $d 3]
      output_append "    space=$space errno=$errno" tag_red
      output_append "    error reading trigger module: $errno ([strerror $space $errno])" tag_red
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
      output_append "    error during select: $errno ([strerror $space $errno])" tag_red
    }
    5 {
      set errno [lindex $d 3]
      output_append "    short read: $errno ([strerror $space $errno])" tag_red
    }
    6 {
      output_append "    eventcounter mismatch" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      set evc [lindex $d 5]
      output_append "    [format {mbx=0x%08x state=0x%08x evc=%d} $mbx $state $evc]" tag_red
      output_append "    [decode_syncstatus $state]" tag_red
    }
    7 {
      output_append "    subevent invalid" tag_red
      set mbx [lindex $d 3]
      set state [lindex $d 4]
      output_append "    [format {mbx=0x%08x state=0x%08x} $mbx $state]" tag_red
      output_append "    [decode_syncstatus $state]" tag_red
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
    10 {
      set state [lindex $d 3]
      output_append "    [format {bad state: 0x%04x} $state]" tag_red
    }
    101 {
        output_append "sis3100_zelsync: cannot read latch register" tag_red
    }
    102 {
        output_append "sis3100_zelsync: no event number received" tag_red
    }
    103 {
        set wrong [lindex $d 3]
        output_append "sis3100_zelsync: illegal event number $wrong received" tag_red
    }
    default {
      output_append "    unknown subcode $subcode" tag_red
    }
  }
  return 1
}
