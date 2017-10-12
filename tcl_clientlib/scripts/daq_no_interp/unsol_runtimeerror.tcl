# $Id: unsol_runtimeerror.tcl,v 1.1 2000/08/31 15:43:12 wuestner Exp $
# copyright 2000
#   Peter Wuestner; Zentrallabor fuer Elektronik; Forschungszentrum Juelich
#

proc unsol_runtimeerror {space v h d} {
  output "VED [$v name] ([set vedsetup_${space}::description]): runtimeerror"  tag_red
  output_append "   data: $d"  tag_red
  set code [lindex $d 0]
  # 1: OutDev
  # 2: Trig
  # 3: Mismatch
  # 4: ExecProcList
  # 5: Other
  # 6: InDev
  switch $code {
    1 {set res [decode_unsol_runtime_OutDev $space $v $h $d]}
    2 {set res [decode_unsol_runtime_Trig $space $v $h $d]}
    3 {set res [decode_unsol_runtime_Mismatch $space $v $h $d]}
    4 {set res [decode_unsol_runtime_ExecProcList $space $v $h $d]}
    5 {set res [decode_unsol_runtime_Other $space $v $h $d]}
    6 {set res [decode_unsol_runtime_InDev $space $v $h $d]}
    default output_append "  unknown errorcode $code" tag_red
  }
  if {$res>0} change_status_fatal
}
