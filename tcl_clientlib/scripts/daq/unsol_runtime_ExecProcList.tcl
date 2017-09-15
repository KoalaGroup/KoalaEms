# $ZEL: unsol_runtime_ExecProcList.tcl,v 1.6 2005/02/22 17:54:52 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc decode_unsol_runtime_ExecProcList {space v h d} {
  #output_append "  ExecProcList" tag_blue

  #set code [lindex $d 0] == rtErr_ExecProcList
  set eventcnt [lindex $d 1]
  set error [lindex $d 2]
  set isid [lindex $d 3]
  set trigid [lindex $d 4]
  set l [llength $d]
  incr l -1
  set procerr [lindex $d $l]
  incr l -1
  set procnum [lindex $d $l]
  incr l -1
  output_append "  [ems_errcode $error]: event $eventcnt, ISid $isid, \
trigger $trigid:" tag_red
  output_append "    procedure $procnum, error $procerr ([ems_plcode $procerr])" tag_red
  output_append "    generated data: [lrange $d 5 $l]" tag_red
  set islist [$v namelist is]
  set iss 0
  foreach is $islist {
    set id [$v is id $is]
    if {$id==$isid} {
      set is_idx $is
      incr iss
    }
  }
  if {$iss>1} {
    output_append "    There is more than one IS with ID $isid!" tag_red
    return 1
  }
  if {$iss<1} {
    output_append "    Cannot find IS with ID $isid!" tag_red
    return 1
  }

  set is [$v is open $is_idx]
  set plist [$is readoutlist upload $trigid]
  output_append "    proclist: $plist" tag_red
  $is close

  return 1
}
