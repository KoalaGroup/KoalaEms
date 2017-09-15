proc p {text liste} {
  output "$text: $liste" tag_orange
}

proc wincc_vars {write_arr read_arr} {
  global global_wcc verbose
  upvar write_arr warr
  upvar read_arr rarr
  
  if {$global_wcc(path)==-1} {
    if [connect_to_server] return -1
  }
  set command {}
  lappend command $global_wcc(ved) command1 WCC_RW $global_wcc(path)
  output_append "1 $command"
  foreach i [lsort -integer [array names warr]] {
    output_append "name : [lindex $warr($i) 0]"
    output_append "type : [lindex $warr($i) 1]"
    output_append "value: [lindex $warr($i) 2]"
    set type [lindex $warr($i) 1]
    set val [lindex $warr($i) 2]
    # name of variable
    lappend command [string2xdr [lindex $warr($i) 0]]
  output_append "a $command"
    # code for write
    lappend command 1
  output_append "b $command"
    lappend command [string2xdr $type]
  output_append "c $command"
    switch $type {
      bit   -
      byte  -
      word  -
      dword -
      sbyte -
      sword {
        lappend command $val
      }
      char {
        output_append "add char"
        lappend command [string2xdr $val]
      }
      double {
        lappend command [double2xdr $val]
      }
      float {
        lappend command [float2xdr $val]
      }
      raw {
        output "raw not converted" tag_red
        return -1
      }
      default {
        output "type $typ is not known" tag_red
        return -1
      }
    }
  output_append "2 $command"
  }
  output "command for write:"
  output_append $command
  foreach i [lsort -integer [array names rarr]] {
    output_append "name : [lindex $rarr($i) 0]"
    output_append "type : [lindex $rarr($i) 1]"
    lappend command [string2xdr [lindex $rarr($i) 0]] 0 \
        [string2xdr [lindex $rarr($i) 1]]
  }
  output "command for write and read:"
  output_append $command
  if [catch {
    set res [eval [join $command]]
  } mist] {
    output "WCC_RW: $mist" tag_red
    return
  }
  output "result:"
  output_append $res
}
