#
# $Id: wincc_var.tcl,v 1.1 2000/07/15 21:34:41 wuestner Exp $
# © 1999 P. Wüstner; Zentrallabor für Elektronik; Forschungszentrum Jülich
#

proc read_wincc_var {typ name wincc_err var_err} {
  global global_wcc verbose
  upvar wincc_err wccerr
  upvar var_err varerr

  if {$global_wcc(path)==-1} {
    if [connect_to_server] return ""
  }
  if [catch {
    set res [$global_wcc(ved) command1 WCC_R $global_wcc(path) '$name' '$typ']
  } mist] {
    output "WCC_R: $mist" tag_red
    set wccerr -1
    return "";
  }
#  output "res: $res" tag_orange
  set wccerr [lindex $res 1]
  if {$wccerr!=0} {
    output "WCC_error: $wccerr" tag_red
    return "";
  }
  set varerr [lindex $res 2]
  if {$varerr!=0} {
    output "VAR_error: $varerr" tag_red
    return "";
  }
  set rest [lrange $res 3 end]
  set val 0
  switch $typ {
    bit   -
    byte  -
    word  -
    dword -
    sbyte -
    sword {
      set val [lindex $rest 0]
    }
    char {
      xdr2string $rest val
    }
    double {
      set val [eval xdr2double $rest]
    }
    float {
      set val [eval xdr2float $rest]
    }
    raw {
      output "raw not converted" tag_red
    }
    default {
      output "type $typ is not known" tag_red
      return ""
    }
  }
  output "read  variable $typ $name: $val"
  return $val
}

# void
proc write_wincc_var {typ name val wincc_err var_err} {
  global global_wcc verbose
  upvar wincc_err wccerr
  upvar var_err varerr

  if {$global_wcc(path)==-1} {
    if [connect_to_server] return ""
  }

  switch $typ {
    bit   -
    byte  -
    word  -
    dword -
    sbyte -
    sword {
      set arg $val
    }
    char {
      set arg [string2xdr $val]
    }
    double {
      set arg [double2xdr $val]
    }
    float {
      set arg [float2xdr $val]
    }
    raw {
      output "raw not converted" tag_red
      return
    }
    default {
      output "type $typ is not known" tag_red
      return
    }
  }

  if [catch {
    set res [eval $global_wcc(ved) command1 WCC_W $global_wcc(path) '$name' '$typ' $arg]
  } mist] {
    output "WCC_RW: $mist" tag_red
  } else {
  #  output "res: $res" tag_orange
    set wccerr [lindex $res 1]
    if {$wccerr!=0} {
      output "WCC_error: $wccerr" tag_red
      return
    }
    set varerr [lindex $res 2]
    if {$varerr!=0} {
      output "VAR_error: $varerr" tag_red
      return
    }
    output "wrote variable $typ $name: $val"
  }
}
