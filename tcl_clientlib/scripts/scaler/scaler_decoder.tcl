# $ZEL: scaler_decoder.tcl,v 1.11 2011/11/26 23:58:47 wuestner Exp $
# copyright 1998 2006
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
# global vars:
# 
# global_timestamp // timestamp updated by got_Timestamp und used by the
#                  // scaler decoders
# scaler_time
# scaler_time_last
# scaler_cont
# scaler_cont_last
# ro_status ro_status_last
# 

proc got_Timestamp {idx offs num jetzt rest} {
  upvar $rest list
  global global_timestamp

  set day [lindex $list 0]
  set sec [lindex $list 1]
  set hund [lindex $list 2]
  set global_timestamp [expr $sec+$hund/100.0]

  set list [lrange $list 3 end]
  return 0
}

proc got_ScalerU32 {idx offs num jetzt rest} {
  upvar $rest list
  global global_timestamp
  global scaler_time
  global scaler_time_last
  global scaler_cont
  global scaler_cont_last

  if {[llength $list]<$num} {
    output "got_ScalerU32: need $num words for $num channels but got only \
[llength $list]"
    output "got: idx=$idx; offs=$offs list=$list"
    return -1
  }
  for {set i 0} {$i<$num} {incr i} {
    set scaler_cont_last($offs) $scaler_cont($offs)
    set scaler_cont($offs) [lindex $list $i]
    set scaler_time_last($offs) $scaler_time($offs)
    set scaler_time($offs) $global_timestamp
    incr offs
  }

  set list [lrange $list $num end]
  return 0
}

# num has to be a multiple of 32!
# remark:
# in an older version (ikplab01:/usr/local/ems/lib.old/... there was a
# modification:
# if $mod<4 the term "expr [lindex $list $i]*4294967296.0" was removed.
# This was necessary only because the server code did not handle the
# overflows correctly. (it assumes an overflow if the scaler value was 
# smaller than the last one. But the lowest six bit are not reliable
# and have to be ignored for this test.
#
# 2nd remark:
# the 1st remark ist wrong.
# At WASA the fifth scaler is resetted for every spill. This
# makes the overflow detection impossible (each reset will be seen
# as an overflow).
# The problem should be solved at server level but we can have a dirty
# workaround here:
# Because of the resets a real overflow is unlikely to occur, we can 
# just ignore the upper 32 bit of the value (whitch are wrong).

proc got_sis3800ShadowUpdate {idx offs num jetzt rest} {
    upvar $rest list
    global global_timestamp
    global scaler_time
    global scaler_time_last
    global scaler_cont
    global scaler_cont_last

    set wasa_exception_modules {4}

    set modules [expr $num/32]
    set expected_length [expr 1+$modules*(64+1)]

    if {[llength $list]<$expected_length} {
        output "got_sis3800ShadowUpdate: need $expected_length words for \
$num channels but got only [llength $list]"
        output "got: idx=$idx; offs=$offs list=$list"
        return -1
    }
    if {$modules!=[lindex $list 0]} {
        output "got_sis3800ShadowUpdate: $num modules expected but got \
[lindex $list 0]"
        output "got: idx=$idx; offs=$offs list=$list"
        return -1
    }
    set list [lrange $list 1 end] ;# skip module number
    for {set mod 0} {$mod<$modules} {incr mod} {
        set list [lrange $list 1 end] ;# skip channel number
        for {set i 0} {$i<64} {incr i 2} {
            set scaler_cont_last($offs) $scaler_cont($offs)
            if {[lsearch -exact -integer $wasa_exception_modules $mod]>=0} {
                set scaler_cont($offs) [format {%.0f} \
                    [lindex $list [expr $i+1]]]
            } else {
                set scaler_cont($offs) [format {%.0f} \
                    [expr [lindex $list $i]*4294967296.0\
                    +[lindex $list [expr $i+1]]]]
            }
            set scaler_time_last($offs) $scaler_time($offs)
            set scaler_time($offs) $global_timestamp
            incr offs
        }
        set list [lrange $list 64 end]
    }

    set list [lrange $list $num end]
    return 0
}

proc got_Scaler2551 {idx offs num jetzt rest} {
  upvar $rest list
  global global_timestamp
  global scaler_time
  global scaler_time_last
  global scaler_cont
  global scaler_cont_last


  if {[llength $list]<[expr $num*2+1]} {
    output "got_Scaler2551: need [expr $num*2+1] words for $num channels \
but got only [llength $list]"
    output "got: idx=$idx; offs=$offs list=$list"
    return -1
  }
  if {$num!=[lindex $list 0]} {
    output "got_Scaler2551: $num channels expected but got [lindex $list 0]"
    output "got: idx=$idx; offs=$offs list=$list"
    return -1
  }
  set list [lrange $list 1 end]
  set num [expr 2*$num]
  for {set i 0} {$i<$num} {incr i 2} {
    set scaler_cont_last($offs) $scaler_cont($offs)
    set scaler_cont($offs) [format {%.0f} [expr [lindex $list $i]*4294967296.0\
        +[lindex $list [expr $i+1]]]]
    set scaler_time_last($offs) $scaler_time($offs)
    set scaler_time($offs) $global_timestamp
    incr offs
  }

  set list [lrange $list $num end]
  return 0
}

proc got_Scaler4434 {idx offs num jetzt rest} {
  upvar $rest list
  global global_timestamp
  global scaler_time
  global scaler_time_last
  global scaler_cont
  global scaler_cont_last

  if {[llength $list]<[expr $num+1]} {
    output "got_Scaler4434: need [expr $num+1] words for $num channels but \
got only [llength $list]"
    output "got : idx=$idx; offs=$offs list=$list"
    return -1
  }
  if {$num!=[lindex $list 0]} {
    output "got_Scaler4434: $num channels expected but got [lindex $list 0]"
    output "got : idx=$idx; offs=$offs list=$list"
    return -1
  }
  set list [lrange $list 1 end]
  for {set i 0} {$i<$num} {incr i } {
    set scaler_cont_last($offs) $scaler_cont($offs)
    set scaler_cont($offs) [expr [lindex $list $i] & 0xffffff]
    set scaler_time_last($offs) $scaler_time($offs)
    set scaler_time($offs) $global_timestamp
    incr offs
  }

  set list [lrange $list $num end]
  return 0
}

proc got_Scaler4434_u {idx offs num jetzt rest} {
# ist irgendwie identisch zu 'got_Scaler2551'
  upvar $rest list
  global global_timestamp
  global scaler_time
  global scaler_time_last
  global scaler_cont
  global scaler_cont_last

  if {[llength $list]<[expr $num*2+1]} {
    output "got_Scaler4434_u: need [expr $num*2+1] words for $num channels \
but got only [llength $list]"
    output "got : idx=$idx; offs=$offs list=$list"
    return -1
  }
  if {$num!=[lindex $list 0]} {
    output "got_Scaler4434_u: $num channels expected but got [lindex $list 0]"
    output "got : idx=$idx; offs=$offs list=$list"
    return -1
  }
  
  set list [lrange $list 1 end]
  set num [expr 2*$num]
  for {set i 0} {$i<$num} {incr i 2} {
    set scaler_cont_last($offs) $scaler_cont($offs)
    set scaler_cont($offs) [format {%.0f} [expr [lindex $list $i]*4294967296.0\
        +[lindex $list [expr $i+1]]]]
    set scaler_time_last($offs) $scaler_time($offs)
    set scaler_time($offs) $global_timestamp
    incr offs
  }

  set list [lrange $list $num end]
  return 0
}

proc got_Evrate {idx offs num jetzt rest} {
  upvar $rest list
  global ro_status ro_status_last
  global scaler_time
  global scaler_time_last
  global scaler_cont
  global scaler_cont_last

  if {[llength $list]<3} {
    output "got_Evrate(idx=$idx): 3 words needed, got $list"
    output "got : idx=$idx; offs=$offs list=$list"
    return -1
  }
  set ro_status_last(status) $ro_status(status)
  set ro_status_last(count)  $ro_status(count)
  set ro_status_last(time)   $ro_status(time)
  set ro_status(status) [lindex $list 0]
  set ro_status(count)  [lindex $list 1]
  set ro_status(time)   [lindex $list 2]

  set scaler_cont_last($offs) $scaler_cont($offs)
  set scaler_time_last($offs) $scaler_time($offs)
  set scaler_cont($offs) $ro_status(count)
  set scaler_time($offs) $ro_status(time)

  if {"$ro_status_last(status)"!="$ro_status(status)"} {
    if {("$ro_status_last(status)"=="running") \
        || ("$ro_status_last(status)"=="stopped")} {
      auto_print_scaler
    }
  }

  set list [lrange $list 3 end]
  return 0
}
