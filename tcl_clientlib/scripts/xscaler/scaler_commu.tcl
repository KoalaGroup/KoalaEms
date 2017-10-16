# $ZEL: scaler_commu.tcl,v 1.2 2011/11/27 00:09:17 wuestner Exp $
# copyright 1998-2011
#   P. Wuestner; Zentralinstitut fuer Elektronik; Forschungszentrum Juelich
#
proc connect_xh {} {
  global global_setup global_xh_path

  output "connect to xh: $global_setup(xh_host) $global_setup(xh_port) ..."
  if [catch {socket $global_setup(xh_host) $global_setup(xh_port)} mist] {
    output_append $mist
    puts $mist
    return -1
  }
  set global_xh_path $mist
  if [catch {
        puts $global_xh_path "master scaler"
        flush $global_xh_path
        output_append "ok."
      } mist ] {
    output_append $mist
    return -1
  }
  return 0
}

proc disconnect_xh {} {
  global global_setup global_xh_path

  output "disconnect from xh: $global_setup(xh_host) $global_setup(xh_port) ..."
  close $global_xh_path
  output_append "ok."
  return 0
}
